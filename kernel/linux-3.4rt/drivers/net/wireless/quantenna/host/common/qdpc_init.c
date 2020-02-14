/**
 * Copyright (c) 2012-2012 Quantenna Communications, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "qdpc_config.h"
#include "qdpc_debug.h"
#include "qdpc_init.h"
#include "qdpc_regs.h"
#include "qdpc_platform.h"
#include "topaz_vnet.h"

#ifdef GPL_CODE_QTN_RELOAD
#include <bcmnetlink.h> /* kerSysSendtoMonitorTask */
#include <bcm_map_part.h>
#include <linux/reboot.h>
#endif

#define QDPC_TOPAZ_IMG		"topaz-linux.lzma.img"
#define QDPC_TOPAZ_UBOOT	"u-boot.bin"
#define MAX_IMG_NUM		2

#if defined(GPL_CODE)
/*
 * If there is firmware in the flash, then host driver will load it by default.
 * This is a problem.
 * We should force it always load the firmware on our board.
 */
#define EP_BOOT_FROM_FLASH 0
#else
#define EP_BOOT_FROM_FLASH 1
#endif

#ifndef MEMORY_START_ADDRESS
#define MEMORY_START_ADDRESS virt_to_bus((void *)PAGE_OFFSET)
#endif

static unsigned int tlp_mps = 256;
module_param(tlp_mps, uint, 0644);
MODULE_PARM_DESC(tlp_mps, "Default PCIe Max_Payload_Size");

#define PCIE_HOTPLUG_SUPPORTED

/* Quantenna PCIE vendor and device identifiers  */
static struct pci_device_id qdpc_pcie_ids[] = {
	{PCI_DEVICE(QDPC_VENDOR_ID, QDPC_DEVICE_ID),},
	{0,}
};

MODULE_DEVICE_TABLE(pci, qdpc_pcie_ids);

static int qdpc_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void qdpc_pcie_remove(struct pci_dev *pdev);
static int qdpc_boot_thread(void *data);
static void qdpc_nl_recv_msg(struct sk_buff *skb);
int qdpc_init_netdev(struct net_device  **net_dev, struct pci_dev *pdev);

static bool is_ep_reset = false;
#ifndef PCIE_HOTPLUG_SUPPORTED
static int link_monitor(void *data);
static struct task_struct *link_monitor_thread = NULL;
#endif

#ifdef GPL_CODE_QTN_RELOAD
static int aei_qtn_reload_monitor(void *data);
static struct task_struct *aei_qtn_reload_thread = NULL;
#endif

char qdpc_pcie_driver_name[] = "qdpc_host";

static struct pci_driver qdpc_pcie_driver = {
	.name     = qdpc_pcie_driver_name,
	.id_table = qdpc_pcie_ids,
	.probe    = qdpc_pcie_probe,
	.remove   = qdpc_pcie_remove,
#ifdef CONFIG_QTN_PM
	.suspend  = qdpc_pcie_suspend,
	.resume  = qdpc_pcie_resume,
#endif
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
struct netlink_kernel_cfg qdpc_netlink_cfg = {
	.groups   = 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	.flags    = 0,
#endif
	.input    = qdpc_nl_recv_msg,
	.cb_mutex = NULL,
	.bind     = NULL,
};
#endif

struct sock *qdpc_nl_sk = NULL;
int qdpc_clntPid = 0;

unsigned int (*qdpc_pci_readl)(void *addr) = qdpc_readl;
void (*qdpc_pci_writel)(unsigned int val, void *addr) = qdpc_writel;

static int qdpc_bootpoll(struct vmac_priv *p, uint32_t state)
{
	while (!kthread_should_stop() && (qdpc_isbootstate(p,state) == 0)) {
		if (qdpc_booterror(p))
			return -1;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(QDPC_SCHED_TIMEOUT);
	}
	return 0;
}
static void booterror(qdpc_pcie_bda_t *bda)
{
	if (PCIE_BDA_TARGET_FWLOAD_ERR & qdpc_pci_readl(&bda->bda_flags))
		printk("EP boot from download firmware failed!\n");
	else if (PCIE_BDA_TARGET_FBOOT_ERR & qdpc_pci_readl(&bda->bda_flags))
		printk("EP boot from flash failed! Please check if there is usable image in Target flash.\n");
	else
		printk("EP boot get in error, dba flag: 0x%x\n", qdpc_pci_readl(&bda->bda_flags));
}

static void qdpc_pci_endian_detect(struct vmac_priv *priv)
{
	__iomem qdpc_pcie_bda_t *bda = priv->bda;
	volatile uint32_t pci_endian;

	writel(QDPC_PCI_ENDIAN_DETECT_DATA, &bda->bda_pci_endian);
	mmiowb();
	writel(QDPC_PCI_ENDIAN_VALID_STATUS, &bda->bda_pci_pre_status);

	while (readl(&bda->bda_pci_post_status) != QDPC_PCI_ENDIAN_VALID_STATUS) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(QDPC_SCHED_TIMEOUT);
	}

	pci_endian = readl(&bda->bda_pci_endian);
	if (pci_endian == QDPC_PCI_LITTLE_ENDIAN) {
		qdpc_pci_readl = qdpc_readl;
		qdpc_pci_writel = qdpc_writel;
		printk("PCI memory is little endian\n");
	} else if (pci_endian == QDPC_PCI_BIG_ENDIAN) {
		qdpc_pci_readl = qdpc_le32_readl;
		qdpc_pci_writel = qdpc_le32_writel;
		printk("PCI memory is big endian\n");
	} else {
		qdpc_pci_readl = qdpc_readl;
		qdpc_pci_writel = qdpc_writel;
		printk("PCI memory endian value:%08x is invalid - using little endian\n", pci_endian);
	}

	/* Clear endian flags */
	writel(0, &bda->bda_pci_pre_status);
	writel(0, &bda->bda_pci_post_status);
	writel(0, &bda->bda_pci_endian);
}

static void qdpc_pci_dma_offset_reset(struct vmac_priv *priv)
{
	__iomem qdpc_pcie_bda_t *bda = priv->bda;
	uint32_t dma_offset;

	/* Get EP Mapping address */
	dma_offset = readl(&bda->bda_dma_offset);
	if ((dma_offset & PCIE_DMA_OFFSET_ERROR_MASK) != PCIE_DMA_OFFSET_ERROR) {
		printk("DMA offset : 0x%08x, no need to reset the value.\n", dma_offset);
		return;
	}
	dma_offset &= ~PCIE_DMA_OFFSET_ERROR_MASK;

	printk("EP map start addr : 0x%08x, Host memory start : 0x%08x\n",
			dma_offset, (unsigned int)MEMORY_START_ADDRESS);

	/* Reset DMA offset in bda */
	dma_offset -= MEMORY_START_ADDRESS;
	writel(dma_offset, &bda->bda_dma_offset);
}

static int qdpc_firmware_load(struct pci_dev *pdev, struct vmac_priv *priv, const char *name)
{
#define DMABLOCKSIZE	(1 * 1024 * 1024)
#define NBLOCKS(size)  ((size)/(DMABLOCKSIZE) + (((size)%(DMABLOCKSIZE) > 0) ? 1 : 0))

	int result = SUCCESS;
	const struct firmware *fw;
	__iomem qdpc_pcie_bda_t  *bda = priv->bda;

	/* Request compressed firmware from user space */
	if ((result = request_firmware(&fw, name, &pdev->dev)) == -ENOENT) {
		/*
		 * No firmware found in the firmware directory, skip firmware downloading process
		 * boot from flash directly on target
		 */
		printk( "no firmware found skip fw downloading\n");
		qdpc_pcie_posted_write((PCIE_BDA_HOST_NOFW_ERR |
					qdpc_pci_readl(&bda->bda_flags)), &bda->bda_flags);
		return FAILURE;
	} else if (result == SUCCESS) {
		uint32_t nblocks = NBLOCKS(fw->size);
		uint32_t remaining = fw->size;
		uint32_t count;
		uint32_t dma_offset = qdpc_pci_readl(&bda->bda_dma_offset);
		void *data =(void *) __get_free_pages(GFP_KERNEL | GFP_DMA,
			get_order(DMABLOCKSIZE));
		const uint8_t *curdata = fw->data;
		dma_addr_t handle = 0;

		if (!data) {
			printk(KERN_ERR "Allocation failed for memory size[%u] Download firmware failed!\n", DMABLOCKSIZE);
			release_firmware(fw);
			qdpc_pcie_posted_write((PCIE_BDA_HOST_MEMALLOC_ERR |
				qdpc_pci_readl(&bda->bda_flags)), &bda->bda_flags);
			return FAILURE;
		}

		handle = pci_map_single(priv->pdev, data ,DMABLOCKSIZE, PCI_DMA_TODEVICE);
		if (!handle) {
			printk("Pci map for memory data block 0x%p error, Download firmware failed!\n", data);
			free_pages((unsigned long)data, get_order(DMABLOCKSIZE));
			release_firmware(fw);
			qdpc_pcie_posted_write((PCIE_BDA_HOST_MEMMAP_ERR |
				qdpc_pci_readl(&bda->bda_flags)), &bda->bda_flags);
			return FAILURE;
		}

		qdpc_setbootstate(priv, QDPC_BDA_FW_HOST_LOAD);
		qdpc_bootpoll(priv, QDPC_BDA_FW_EP_RDY);

		/* Start loading firmware */
		for (count = 0 ; count < nblocks; count++)
		{
			uint32_t size = (remaining > DMABLOCKSIZE) ? DMABLOCKSIZE : remaining;

			memcpy(data, curdata, size);
			/* flush dcache */
			pci_dma_sync_single_for_device(priv->pdev, handle ,size, PCI_DMA_TODEVICE);

			qdpc_pcie_posted_write(handle + dma_offset, &bda->bda_img);
			qdpc_pcie_posted_write(size, &bda->bda_img_size);
			printk("FW Data[%u]: VA:0x%p PA:0x%p Sz=%u..\n", count, (void *)curdata, (void *)handle, size);

			qdpc_setbootstate(priv, QDPC_BDA_FW_BLOCK_RDY);
			qdpc_bootpoll(priv, QDPC_BDA_FW_BLOCK_DONE);

			remaining = (remaining < size) ? remaining : (remaining - size);
			curdata += size;
			printk("done!\n");
		}

		pci_unmap_single(priv->pdev,handle, DMABLOCKSIZE, PCI_DMA_TODEVICE);
		/* Mark end of block */
		qdpc_pcie_posted_write(0, &bda->bda_img);
		qdpc_pcie_posted_write(0, &bda->bda_img_size);
		qdpc_setbootstate(priv, QDPC_BDA_FW_BLOCK_RDY);
		qdpc_bootpoll(priv, QDPC_BDA_FW_BLOCK_DONE);

		qdpc_setbootstate(priv, QDPC_BDA_FW_BLOCK_END);

		PRINT_INFO("Image. Sz:%u State:0x%x\n", (uint32_t)fw->size, qdpc_pci_readl(&bda->bda_bootstate));
		qdpc_bootpoll(priv, QDPC_BDA_FW_LOAD_DONE);

		free_pages((unsigned long)data, get_order(DMABLOCKSIZE));
		release_firmware(fw);
		PRINT_INFO("Image downloaded....!\n");
	} else {
		PRINT_ERROR("Failed to load firmware:%d\n", result);
		return result;
     }
	return result;
}

static void qdpc_pcie_dev_init(struct vmac_priv *priv, struct pci_dev *pdev, struct net_device *ndev)
{
	SET_NETDEV_DEV(ndev, &pdev->dev);

	priv->pdev = pdev;
	priv->ndev = ndev;
	pci_set_drvdata(pdev, ndev);
}

static void qdpc_tune_pcie_mps(struct pci_dev *pdev, int pos)
{
	struct pci_dev *parent = NULL;
	int ppos = 0;
	uint32_t dev_cap, pcap;
	uint16_t dev_ctl, pctl;
	unsigned int mps = tlp_mps;
#define BIT_TO_MPS(m) (1 << ((m) + 7))

	if (pdev->bus && pdev->bus->self) {
		parent = pdev->bus->self;
		if (likely(parent)) {
			ppos = pci_find_capability(parent, PCI_CAP_ID_EXP);
			if (ppos) {
				pci_read_config_dword(parent, ppos + PCI_EXP_DEVCAP, &pcap);
				pci_read_config_dword(pdev, pos + PCI_EXP_DEVCAP, &dev_cap);
				printk(KERN_INFO "parent cap:%u, dev cap:%u\n",\
						BIT_TO_MPS(pcap & PCI_EXP_DEVCAP_PAYLOAD), BIT_TO_MPS(dev_cap & PCI_EXP_DEVCAP_PAYLOAD));
				mps = min(BIT_TO_MPS(dev_cap & PCI_EXP_DEVCAP_PAYLOAD), BIT_TO_MPS(pcap & PCI_EXP_DEVCAP_PAYLOAD));
			}
		}
	}
#if defined(GPL_CODE_MPS128)
        mps = 128;
#endif
	printk(KERN_INFO"Setting MPS to %u\n", mps);

	/*
	* Set Max_Payload_Size
	* Max_Payload_Size_in_effect = 1 << ( ( (dev_ctl >> 5) & 0x07) + 7);
	*/
	mps = (((mps >> 7) - 1) << 5);
	pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &dev_ctl);
	dev_ctl = ((dev_ctl & ~PCI_EXP_DEVCTL_PAYLOAD) | mps);
	pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, dev_ctl);

	if (parent && ppos) {
		pci_read_config_word(parent, pos + PCI_EXP_DEVCTL, &pctl);
		pctl = ((pctl & ~PCI_EXP_DEVCTL_PAYLOAD) | mps);
		pci_write_config_word(parent, pos + PCI_EXP_DEVCTL, pctl);
	}
}

static struct net_device *g_ndev = NULL;
static int qdpc_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct vmac_priv *priv = NULL;
	struct net_device *ndev = NULL;
	int result = SUCCESS;
	int pos;

	/* Allocate device structure */
	if (!(ndev = vmac_alloc_ndev()))
		return -ENOMEM;

	g_ndev = ndev;
	priv = netdev_priv(ndev);
	qdpc_pcie_dev_init(priv, pdev, ndev);

	/* Check if the device has PCI express capability */
	pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
	if (!pos) {
		PRINT_ERROR(KERN_ERR "The device %x does not have PCI Express capability\n",
	                pdev->device);
		result = -ENOSYS;
		goto out;
	} else {
		PRINT_DBG(KERN_INFO "The device %x has PCI Express capability\n", pdev->device);
	}

	qdpc_tune_pcie_mps(pdev, pos);

	/*  Wake up the device if it is in suspended state and allocate IO,
	 *  memory regions and IRQ if not
	 */
	if (pci_enable_device(pdev)) {
		PRINT_ERROR(KERN_ERR "Failed to initialize PCI device with device ID %x\n",
				pdev->device);

		result = -EIO;
		goto out;
	} else {
		PRINT_DBG(KERN_INFO "Initialized PCI device with device ID %x\n", pdev->device);
	}

	/*
	 * Check if the PCI device can support DMA addressing properly.
	 * The mask gives the bits that the device can address
	 */
	pci_set_master(pdev);

	/* Initialize PCIE layer  */
	if (( result = qdpc_pcie_init_intr_and_mem(priv)) < 0) {
		PRINT_DBG("Interrupt & Memory Initialization failed \n");
		goto release_memory;
	}

	if (!!(result = vmac_net_init(pdev))) {
		PRINT_DBG("Vmac netdev init fail\n");
		goto free_mem_interrupt;
	}

	/* Create and start the thread to initiate the INIT Handshake*/
	priv->init_thread = kthread_run(qdpc_boot_thread, priv, "qdpc_init_thread");
	if (priv->init_thread == NULL) {
		PRINT_ERROR("Init thread creation failed \n");
		goto free_mem_interrupt;
	}

	/* Create netlink & register with kernel */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	priv->nl_socket = netlink_kernel_create(&init_net,
				QDPC_NETLINK_RPC_PCI_CLNT, &qdpc_netlink_cfg);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
	priv->nl_socket = netlink_kernel_create(&init_net,
				QDPC_NETLINK_RPC_PCI_CLNT, THIS_MODULE, &qdpc_netlink_cfg);
#else
	priv->nl_socket = netlink_kernel_create(&init_net,
					QDPC_NETLINK_RPC_PCI_CLNT, 0, qdpc_nl_recv_msg,
					NULL, THIS_MODULE);
#endif
	if (priv->nl_socket) {
		return SUCCESS;
	}

	PRINT_ERROR(KERN_ALERT "Error creating netlink socket.\n");
	result = FAILURE;

free_mem_interrupt:
	qdpc_pcie_free_mem(pdev);
	qdpc_free_interrupt(pdev);

release_memory:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
	/* Releasing the memory region if any error occured */
	pci_clear_master(pdev);
#endif

	pci_disable_device(pdev);

out:
	free_netdev(ndev);
	/* Any failure in probe, so it can directly return in remove */
	pci_set_drvdata(pdev, NULL);

	return result;
}

static void qdpc_pcie_remove(struct pci_dev *pdev)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	struct vmac_priv *vmp;

	if (ndev == NULL)
		return;

	vmp = netdev_priv(ndev);

	if (vmp->init_thread)
		kthread_stop(vmp->init_thread);
	if (vmp->nl_socket)
		netlink_kernel_release(vmp->nl_socket);

	vmac_clean(ndev);

	qdpc_free_interrupt(pdev);
	qdpc_pcie_free_mem(pdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
	pci_clear_master(pdev);
#endif
	pci_disable_device(pdev);

	writel(TOPAZ_SET_INT(IPC_RESET_EP), vmp->ep_ipc_reg);
	qdpc_unmap_iomem(vmp);

	free_netdev(ndev);
	g_ndev = NULL;

	return;
}

static inline int qdpc_pcie_set_power_state(struct pci_dev *pdev, pci_power_t state)
{
	uint32_t pmcsr;

	pci_read_config_dword(pdev, TOPAZ_PCI_PM_CTRL_OFFSET, &pmcsr);

	switch (state) {
	case PCI_D0:
		pci_write_config_dword(pdev, TOPAZ_PCI_PM_CTRL_OFFSET,(pmcsr & ~PCI_PM_CTRL_STATE_MASK) | PCI_D0);
		break;

	case PCI_D3hot:
		pci_write_config_dword(pdev, TOPAZ_PCI_PM_CTRL_OFFSET,(pmcsr & ~PCI_PM_CTRL_STATE_MASK) | (PCI_D3hot | PCI_PM_CTRL_PME_ENABLE));
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int qdpc_pcie_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	struct vmac_priv *priv;

	if (ndev == NULL)
		return -EINVAL;

	priv = netdev_priv(ndev);
	if (le32_to_cpu(*priv->ep_pmstate) == PCI_D3hot) {
		return 0;
	}

	printk("%s start power management suspend\n", qdpc_pcie_driver_name);

	/* Set ep not ready to drop packets in low power mode */
	priv->ep_ready = 0;
	ndev->flags &= ~IFF_RUNNING;
	*priv->ep_pmstate = cpu_to_le32(PCI_D3hot);
	barrier();
	writel(TOPAZ_SET_INT(IPC_EP_PM_CTRL), priv->ep_ipc_reg);

	msleep(100);
	pci_save_state(pdev);
	pci_disable_device(pdev);
	qdpc_pcie_set_power_state(pdev, PCI_D3hot);

	return 0;
}

int qdpc_pcie_resume(struct pci_dev *pdev)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	struct vmac_priv *priv;
	int ret;

	if (ndev == NULL)
		return -EINVAL;

	priv = netdev_priv(ndev);
	if (le32_to_cpu(*priv->ep_pmstate) == PCI_D0) {
		return 0;
	}

	printk("%s start power management resume\n", qdpc_pcie_driver_name);

	ret = pci_enable_device(pdev);
	if (ret) {
		PRINT_ERROR("%s: pci_enable_device failed on resume\n", __func__);
		return ret;
	}

	pci_restore_state(pdev);
	qdpc_pcie_set_power_state(pdev, PCI_D0);

	*priv->ep_pmstate = cpu_to_le32(PCI_D0);
	barrier();
	writel(TOPAZ_SET_INT(IPC_EP_PM_CTRL), priv->ep_ipc_reg);

	msleep(5000);
	/* Set ep_ready to resume tx traffic */
	priv->ep_ready = 1;
	ndev->flags |= IFF_RUNNING;

	return 0;
}
#if defined(GPL_CODE_QTN_RELOAD)

#define AEI_PCIE_REINIT_PORT_QDPC      1    //port 1 for QDPC module
#define AEI_PCIE_REINIT_POWER_UP    1
#define AEI_PCIE_REINIT_POWER_DOWN  0

extern void bcm63xx_pcie_aloha(int hello);
extern uint32_t aei_bcm63xx_pcie_reinit_port ;

/*
* Function: reinit pcie configration
* parameter: init
*        0: power down pcie 
*        1: first invocation: save pci configuration
*           subsequent: repower and restore configuration
*/
void aei_qdpc_pcie_reinit(int init)
{
    /*  set pcie port number to reinit */
    aei_bcm63xx_pcie_reinit_port = AEI_PCIE_REINIT_PORT_QDPC;
    
    /*  reinit pcie configration */
    bcm63xx_pcie_aloha(init);

    /*  clear pcie port number to reinit */
    aei_bcm63xx_pcie_reinit_port = 0;
}
#endif

static int __init qdpc_init_module(void)
{
	int ret;

	PRINT_DBG(KERN_INFO "Quantenna pcie driver initialization\n");

#if defined(GPL_CODE_QTN_RELOAD)
    aei_qdpc_pcie_reinit(AEI_PCIE_REINIT_POWER_UP);
#endif

	if (qdpc_platform_init()) {
		PRINT_ERROR("Platform initilization failed \n");
		ret = FAILURE;
		return ret;
	}

	/*  Register the pci driver with device*/
	if ((ret = pci_register_driver(&qdpc_pcie_driver)) < 0 ) {
		PRINT_ERROR("Could not register the driver to pci : %d\n", ret);
		ret = -ENODEV;
		return ret;
	}

#ifndef PCIE_HOTPLUG_SUPPORTED
	link_monitor_thread = kthread_run(link_monitor, NULL, "link_monitor");
#endif

#ifdef GPL_CODE_QTN_RELOAD
    aei_qtn_reload_thread = kthread_run(aei_qtn_reload_monitor, NULL, "qtn_reload_monitor");
    if (aei_qtn_reload_thread == NULL) {
        PRINT_ERROR("Init qtn_reload_thread creation failed \n");
        ret = FAILURE;
        return ret;
    }
#endif
	return ret;
}

static void __exit qdpc_exit_module(void)
{
	/* Release netlink */
	qdpc_platform_exit();

#if defined(GPL_CODE_QTN_RELOAD)
    aei_qdpc_pcie_reinit(AEI_PCIE_REINIT_POWER_DOWN);
    aei_qdpc_pcie_reinit(AEI_PCIE_REINIT_POWER_UP);
#endif

#ifndef PCIE_HOTPLUG_SUPPORTED
	kthread_stop(link_monitor_thread);
	link_monitor_thread = NULL;
#endif

#ifdef GPL_CODE_QTN_RELOAD
    if(aei_qtn_reload_thread){
        kthread_stop(aei_qtn_reload_thread);
    }
    aei_qtn_reload_thread = NULL;
#endif
	/* Unregister the pci driver with the device */
	pci_unregister_driver(&qdpc_pcie_driver);

	return;
}

static inline bool is_pcie_linkup(struct pci_dev *pdev)
{
	uint32_t cs = 0;

	pci_read_config_dword(pdev, QDPC_VENDOR_ID_OFFSET, &cs);
	if (cs == QDPC_LINK_UP) {
		msleep(10000);
		printk("%s: PCIe link up!\n", __func__);
		return true;
	}

	return false;
}

static inline void qdpc_pcie_print_config_space(struct pci_dev *pdev)
{
	int i = 0;
	uint32_t cs = 0;

	/* Read PCIe configuration space header */
	for (i = QDPC_VENDOR_ID_OFFSET; i <= QDPC_INT_LINE_OFFSET; i += QDPC_ROW_INCR_OFFSET) {
		pci_read_config_dword(pdev, i, &cs);
		printk("%s: pdev:0x%p config_space offset:0x%02x value:0x%08x\n", __func__, pdev, i, cs);
	}
	printk("\n");
}

static inline void qdpc_pcie_check_link(struct pci_dev *pdev, struct vmac_priv *priv)
{
	__iomem qdpc_pcie_bda_t *bda = priv->bda;
	uint32_t cs = 0;

	pci_read_config_dword(pdev, QDPC_VENDOR_ID_OFFSET, &cs);
	/* Endian value will be all 1s if link went down */
	if (readl(&bda->bda_pci_endian) == QDPC_LINK_DOWN) {
		is_ep_reset = true;
		printk("Reset detected\n");
	}
}

#ifndef PCIE_HOTPLUG_SUPPORTED
static int link_monitor(void *data)
{
	struct net_device *ndev = NULL;
	struct vmac_priv *priv = NULL;
	__iomem qdpc_pcie_bda_t *bda = NULL;
	struct pci_dev *pdev = NULL;
	uint32_t cs = 0;

	set_current_state(TASK_RUNNING);
	while (!kthread_should_stop()) {
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		set_current_state(TASK_RUNNING);

		ndev = g_ndev;
		priv = netdev_priv(ndev);
		bda = priv->bda;
		pdev = priv->pdev;

#ifdef QDPC_CS_DEBUG
		qdpc_pcie_print_config_space(pdev);
		msleep(5000);
#endif

		/* Check if reset to EP occurred */
		while (!pci_read_config_dword(pdev, QDPC_VENDOR_ID_OFFSET, &cs)) {

			if (kthread_should_stop())
				do_exit(0);

			qdpc_pcie_check_link(pdev, priv);
			if (is_ep_reset) {
				is_ep_reset = false;
				qdpc_pcie_remove(pdev);
				printk("%s: Attempting to recover from EP reset\n", __func__);
				break;
			}
			msleep(500);
		}

		while(!is_pcie_linkup(pdev)) {
		}

#ifdef QDPC_CS_DEBUG
		qdpc_pcie_print_config_space(pdev);
#endif

		qdpc_pcie_probe(pdev, NULL);
    }
    do_exit(0);
}
#endif

#ifdef GPL_CODE_QTN_RELOAD
#define AEI_MAX_TRY_TIMES       60
#define AEI_MAX_INTERVAL_TIME   (5*1000)
#define AEI_DETECT_QTN_LOCKUP
#define AEI_DETECT_QTN_REBOOT  (((readl(PCIE_BASE + 0xb8) >> 15) & 0x7) == 0x2)

#ifdef GPL_CODE_DEBUG_QTN_RELOAD
int aei_qtn_reload_flag = 0;
#endif

static int aei_qtn_reload_monitor(void *data)
{
    struct vmac_priv *vmp = netdev_priv(g_ndev);
    uint32_t cur_num=0, last_num = 0;
    uint32_t try_times = 0;
    uint32_t wait_time = AEI_MAX_INTERVAL_TIME; /* normal monitor interval time is 5 Seconds*/

    printk("Start to monitor qtn reload. ..... \n");
    printk("Waiting for EP bootup... \n");
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    set_current_state(TASK_RUNNING);

    printk("Detect EP bootup complete \n");

    while (!kthread_should_stop()) {
        msleep(wait_time);
#ifdef GPL_CODE_DETECT_QTN_REBOOT
        /* check if qtn reboot*/
        if(AEI_DETECT_QTN_REBOOT){
            printk("system detect qtn reboot \n");
            //we have verified that DUT will reboot upon detect qtn reboot
            //you can uncomment code below to reboot DUT
            kernel_restart(NULL);
            return 0;
        }
#endif
#ifdef GPL_CODE_DETECT_QTN_LOCKUP
        /* Get latest queue lenth*/
        cur_num = vmp->vmac_tx_queue_len;
        /* check if queue length changed*/
        if(cur_num != last_num ){
            try_times = 0;
            wait_time = AEI_MAX_INTERVAL_TIME;
        }/* max valid tx queue length is QDPC_TX_QUEUE_SIZE-1, check if queue becomes full for specified times */
        else if(cur_num  >= (QDPC_TX_QUEUE_SIZE-1)){
            try_times ++;
            wait_time = 1000;
        }

        if(try_times >= AEI_MAX_TRY_TIMES
#ifdef GPL_CODE_DEBUG_QTN_RELOAD
             || aei_qtn_reload_flag > 0
#endif
            ){

            printk("System detect tx queue is full(length:%d) and failed to transmit packet to Quantenna card for %d seconds. \n", cur_num,try_times);
            /* Send message to notify SSK to reload QTN host driver */
            printk("Ready to notify SSK to reload QTN host driver\n");
            kerSysSendtoMonitorTask(MSG_NETLINK_AEI_QTN_RELOAD, NULL, 0);

#ifdef GPL_CODE_DEBUG_QTN_RELOAD
            aei_qtn_reload_flag = 0;
#endif
            /* Terminate thread */
            aei_qtn_reload_thread = NULL;
            do_exit(0);
        }
        /* save the current value*/
        last_num = cur_num;
#endif
    }
    return 0;
}
#endif

static int qdpc_bringup_fw(struct vmac_priv *priv)
{
	__iomem qdpc_pcie_bda_t  *bda = priv->bda;
	uint32_t bdaflg;
	char *fwname;

	qdpc_pci_endian_detect(priv);
	qdpc_pci_dma_offset_reset(priv);

	printk("Setting HOST ready...\n");
	qdpc_setbootstate(priv, QDPC_BDA_FW_HOST_RDY);
	qdpc_bootpoll(priv, QDPC_BDA_FW_TARGET_RDY);

	if (qdpc_set_dma_mask(priv)){
		printk("Failed to map DMA mask.\n");
		priv->init_thread = NULL;
		do_exit(-1);
	}

	bdaflg = qdpc_pci_readl(&bda->bda_flags);
	if ((PCIE_BDA_FLASH_PRESENT & bdaflg) && EP_BOOT_FROM_FLASH) {
		printk("EP have fw in flash, boot from flash\n");
		qdpc_pcie_posted_write((PCIE_BDA_FLASH_BOOT |
			qdpc_pci_readl(&bda->bda_flags)), &bda->bda_flags);
		qdpc_setbootstate(priv, QDPC_BDA_FW_TARGET_BOOT);
		qdpc_bootpoll(priv, QDPC_BDA_FW_FLASH_BOOT);
		goto fw_start;
	}
	bdaflg &= PCIE_BDA_XMIT_UBOOT;
	fwname = bdaflg ? QDPC_TOPAZ_UBOOT : QDPC_TOPAZ_IMG;

	qdpc_setbootstate(priv, QDPC_BDA_FW_TARGET_BOOT);
	printk("EP FW load request...\n");
	qdpc_bootpoll(priv, QDPC_BDA_FW_LOAD_RDY);

	printk("Start download Firmware %s...\n", fwname);
	if (qdpc_firmware_load(priv->pdev, priv, fwname)){
		printk("Failed to download firmware.\n");
		priv->init_thread = NULL;
		do_exit(-1);
	}

fw_start:
	qdpc_setbootstate(priv, QDPC_BDA_FW_START);
	printk("Start booting EP...\n");
	if (bdaflg != PCIE_BDA_XMIT_UBOOT) {
		if (qdpc_bootpoll(priv,QDPC_BDA_FW_CONFIG)) {
			booterror(bda);
			priv->init_thread = NULL;
			do_exit(-1);
		}
		printk("EP boot successful, starting config...\n");

		/* Save target-side MSI address for later enable/disable irq*/
		priv->dma_msi_imwr = readl(QDPC_BAR_VADDR(priv->dmareg_bar, TOPAZ_IMWR_DONE_ADDRLO_OFFSET));
		priv->dma_msi_dummy = virt_to_bus(&priv->dma_msi_data) + qdpc_pci_readl(&bda->bda_dma_offset);
		priv->ep_pciecfg0_val = readl(QDPC_BAR_VADDR(priv->sysctl_bar, TOPAZ_PCIE_CFG0_OFFSET));

		qdpc_setbootstate(priv, QDPC_BDA_FW_RUN);
		qdpc_bootpoll(priv,QDPC_BDA_FW_RUNNING);
		priv->ep_ready = 1;
	}

	return (int)bdaflg;
}

static int qdpc_boot_done(struct vmac_priv *priv)
{
	struct net_device *ndev;
	ndev = priv->ndev;

	PRINT_INFO("Connection established with Target BBIC4 board\n");

#ifndef PCIE_HOTPLUG_SUPPORTED
	if (link_monitor_thread)
		wake_up_process(link_monitor_thread);
#endif

#ifdef GPL_CODE_QTN_RELOAD
    if (aei_qtn_reload_thread)
        wake_up_process(aei_qtn_reload_thread);
#endif
	priv->init_thread = NULL;
	do_exit(0);
}

static int qdpc_boot_thread(void *data)
{
	struct vmac_priv *priv = (struct vmac_priv *)data;
	int i;

	for (i = 0; i < MAX_IMG_NUM; i++) {
		if (qdpc_bringup_fw(priv) <= 0)
			break;
	}

	qdpc_boot_done(priv);

	return 0;
}

static void qdpc_nl_recv_msg(struct sk_buff *skb)
{
	struct vmac_priv *priv = netdev_priv(g_ndev);
	struct nlmsghdr *nlh  = (struct nlmsghdr*)skb->data;
	struct sk_buff *skb2;

	/* Parsing the netlink message */

	PRINT_DBG(KERN_INFO "%s line %d Netlink received pid:%d, size:%d, type:%d\n",
		__FUNCTION__, __LINE__, nlh->nlmsg_pid, nlh->nlmsg_len, nlh->nlmsg_type);

	switch (nlh->nlmsg_type) {
		case QDPC_NL_TYPE_CLNT_STR_REG:
		case QDPC_NL_TYPE_CLNT_LIB_REG:
			if (nlh->nlmsg_type == QDPC_NL_TYPE_CLNT_STR_REG)
				priv->str_call_nl_pid = nlh->nlmsg_pid;
			else
				priv->lib_call_nl_pid = nlh->nlmsg_pid;
			return;
		case QDPC_NL_TYPE_CLNT_STR_REQ:
		case QDPC_NL_TYPE_CLNT_LIB_REQ:
			break;
		default:
			PRINT_DBG(KERN_INFO "%s line %d Netlink Invalid type %d\n",
				__FUNCTION__, __LINE__, nlh->nlmsg_type);
			return;
	}

	/*
	 * make a new skb. The old skb will freed in netlink_unicast_kernel,
	 * but we must hold the skb before DMA transfer done
	 */
	skb2 = alloc_skb(nlh->nlmsg_len+sizeof(qdpc_cmd_hdr_t), GFP_ATOMIC);
	if (skb2) {
		qdpc_cmd_hdr_t *cmd_hdr;
		u16 rpc_type = nlh->nlmsg_type & QDPC_RPC_TYPE_MASK;
		cmd_hdr = (qdpc_cmd_hdr_t *)skb2->data;
		memcpy(cmd_hdr->dst_magic, QDPC_NETLINK_DST_MAGIC, ETH_ALEN);
		memcpy(cmd_hdr->src_magic, QDPC_NETLINK_SRC_MAGIC, ETH_ALEN);
		cmd_hdr->type = htons(QDPC_APP_NETLINK_TYPE);
		cmd_hdr->len = htons(nlh->nlmsg_len + sizeof(cmd_hdr->rpc_type));
		cmd_hdr->rpc_type = htons(rpc_type);
		memcpy(skb2->data + sizeof(qdpc_cmd_hdr_t),
				skb->data + sizeof(struct nlmsghdr), nlh->nlmsg_len);
		skb2->dev = priv->ndev;
		skb_put(skb2, nlh->nlmsg_len+sizeof(qdpc_cmd_hdr_t));

		dev_queue_xmit(skb2);
	}
}


module_init(qdpc_init_module);
module_exit(qdpc_exit_module);
