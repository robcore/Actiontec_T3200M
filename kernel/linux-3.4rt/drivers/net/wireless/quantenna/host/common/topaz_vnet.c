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

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/if_bridge.h>
#include <linux/sysfs.h>
#include <linux/pci.h>


#include <qdpc_platform.h>

#include <asm/cache.h>		/* For cache line size definitions */
#include <asm/cacheflush.h>	/* For cache flushing functions */

#include <net/netlink.h>

#include "topaz_vnet.h"
#include "qdpc_config.h"
#include "qdpc_init.h"
#include "qdpc_debug.h"
#include "qdpc_regs.h"
#include "qdpc_version.h"

#if defined(GPL_CODE)
#include <linux/nbuff.h>
#include <linux/blog.h>
#endif

#if defined(GPL_CODE_TOOLBOX)
#include "bcmnet.h"
#endif

#define DRV_NAME	"qdpc-host"

#ifndef DRV_VERSION
#define DRV_VERSION	"1.0"
#endif

#define DRV_AUTHOR	"Quantenna Communications Inc."
#define DRV_DESC	"PCIe virtual Ethernet port driver"

MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");

#undef __sram_text
#define __sram_text

static int __sram_text vmac_rx_poll (struct napi_struct *napi, int budget);
static int __sram_text skb2rbd_attach(struct net_device *ndev, uint16_t i, uint32_t wrap);
static irqreturn_t vmac_interrupt(int irq, void *dev_id);
static void vmac_tx_timeout(struct net_device *ndev);
static int vmac_get_settings(struct net_device *ndev, struct ethtool_cmd *cmd);
static int vmac_set_settings(struct net_device *ndev, struct ethtool_cmd *cmd);
static void vmac_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *info);
static void free_tx_skbs(struct vmac_priv *vmp);
static void init_tx_bd(struct vmac_priv *vmp);
static void free_rx_skbs(struct vmac_priv *vmp);
static int alloc_and_init_rxbuffers(struct net_device *ndev);
static void bring_up_interface(struct net_device *ndev);
static void shut_down_interface(struct net_device *ndev);
static int vmac_open(struct net_device *ndev);
static int vmac_close(struct net_device *ndev);
static int vmac_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd);
#ifdef QTN_WAKEQ_SUPPORT
static inline void vmac_try_wake_queue(struct net_device *ndev);
static inline void vmac_try_stop_queue(struct net_device *ndev);
#endif
#ifdef RX_IP_HDR_REALIGN
static uint32_t align_cnt = 0, unalign_cnt = 0;
#endif


#define RX_DONE_INTR_MSK	((0x1 << 6) -1)
#define VMAC_BD_LEN		(sizeof(struct vmac_bd))

#define QTN_GLOBAL_INIT_EMAC_TX_QUEUE_LEN 256
#define VMAC_DEBUG_MODE
/* Tx dump flag */
#define DMP_FLG_TX_BD		(0x1 << ( 0)) /* vmac s 32 */
#define DMP_FLG_TX_SKB		(0x1 << ( 1)) /* vmac s 33 */
/* Rx dump flag */
#define DMP_FLG_RX_BD		(0x1 << (16)) /* vmac s 48 */
#define DMP_FLG_RX_SKB		(0x1 << (17)) /* vmac s 49 */
#define DMP_FLG_RX_INT		(0x1 << (18)) /* vmac s 50 */

#define SHOW_TX_BD		(16)
#define SHOW_RX_BD		(17)
#define SHOW_VMAC_STATS	(18)


#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
//zli add debug
#define WRITE_RQ_BY_INDEX       (19)
#define IPC_TRIGGER_MANUALLY    (20)
#define READ_TRIGGER_FLAG       (21)
#define SET_TRIGGER_COUNT       (22)

#define RECORD_LENGTH_ZLI       (180)

static uint32_t prev_end_idx,same_idx_cnt= 0;
static int trigger_flag = 0;
static int rewrite = 0;
static int trigger_count = 1000;

static void write_request_queue_by_roundindex(struct vmac_priv *vmp, uint32_t index );
static void ipc_trigger_manually(struct vmac_priv *vmp);

#endif


#ifndef QDPC_PLATFORM_IFPORT
#define QDPC_PLATFORM_IFPORT 0
#endif

#define VMAC_TX_TIMEOUT		(180 * HZ)

#ifdef VMAC_DEBUG_MODE

#define dump_tx_bd(vmp) do { \
		if (unlikely((vmp)->dbg_flg & DMP_FLG_TX_BD)) { \
			txbd2str(vmp); \
		} \
	} while (0)

#define dump_tx_pkt(vmp, data, len) do { \
		if (unlikely(((vmp)->dbg_flg & DMP_FLG_TX_SKB))) \
			dump_pkt(data, len, "Tx"); \
	} while(0)

#define dump_rx_bd(vmp) do { \
		if (unlikely((vmp)->dbg_flg & DMP_FLG_RX_BD)) { \
			rxbd2str(vmp); \
		} \
	} while (0)

#define dump_rx_pkt(vmp, data, len) do { \
		if (unlikely((vmp)->dbg_flg & DMP_FLG_RX_SKB)) \
			dump_pkt(data, len, "Rx"); \
	} while(0)

#define dump_rx_int(vmp) do { \
		if (unlikely((vmp)->dbg_flg & DMP_FLG_RX_INT)) \
			dump_rx_interrupt(vmp); \
	} while (0)

#else
#define dump_tx_bd(vmp)
#define dump_tx_pkt(vmp, skb, len)
#define dump_rx_bd(vmp)
#define dump_rx_pkt(vmp, skb, len)
#define dump_rx_int(vmp)
#endif

struct vmac_cfg vmaccfg = {
	QDPC_RX_QUEUE_SIZE, QDPC_TX_QUEUE_SIZE, "host%d", NULL
};

#if defined(GPL_CODE_TOOLBOX)
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef GPL_CODE_DEBUG_QTN_RELOAD
extern int aei_qtn_reload_flag;
#endif
static char * pMirrorIntfNames[] =
{
    "eth0",
    "eth1",
    "eth2",
    "eth3",
    "eth4", /* for HPNA */
    "wl0",
    "wl0.1",
    "wl0.2",
    "wl0.3",
    "wl1",
    "wl1.1",
    "wl1.2",
    "wl1.3",
    "host0",
    NULL
};

static void AEI_UpdateMirrorFlag(uint16_t *mirFlags, char *intfName, uint8_t enable)
{
    uint8_t i;
    uint16_t flag;

    for (i = 0; pMirrorIntfNames[i]; i++)
    {
        if (strcmp(pMirrorIntfNames[i], intfName) == 0)
        {
            flag = 0x0001 << i;

            if (enable)
                (*mirFlags) |= flag;
            else
                (*mirFlags) &= ~flag;

            break;
        }
    }
}

static void AEI_MirrorPacket( struct sk_buff *skb, char *intfName, bool mirrorIn)
{
    struct sk_buff *skbCopy;
    struct net_device *netDev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    if( (netDev = __dev_get_by_name(&init_net, intfName)) != NULL )
#else
    if( (netDev = __dev_get_by_name(intfName)) != NULL )
#endif
    {
        if( (skbCopy = skb_copy(skb, GFP_ATOMIC)) != NULL )
        {
            unsigned long flags;

            if (mirrorIn)
            {
                unsigned char tmp_addr[ETH_ALEN];
                struct ethhdr * eth_hdr;

                /* when transmiting packets using GW mac address as its ethernet dest mac address,
                 * some version of the BCM53115 switch will drop these packets, so just swap
                 * the ethernet source and dest mac address for these packets.
                 */
                eth_hdr = (struct ethhdr *)skbCopy->data;
                if (eth_hdr)
                {
                   memcpy(tmp_addr, eth_hdr->h_source, ETH_ALEN);
                   memcpy(eth_hdr->h_source, eth_hdr->h_dest, ETH_ALEN);
                   memcpy(eth_hdr->h_dest, tmp_addr, ETH_ALEN);
                }
            }

            skbCopy->dev = netDev;
            skbCopy->blog_p = BLOG_NULL;
            if (strstr(intfName, "wl"))
                skbCopy->protocol = htons(ETH_P_MIRROR_WLAN);//fixme
            else
                skbCopy->protocol = htons(ETH_P_MIRROR);

         /*qtn and 43602 would be convert to a specific station, QTN and 43602 wifi, wireless
            *protocol stack that is on the wifi card, after the mirror, wifi card looks for the
            *destination address, if not found, it will think that there is no client, and don't send out;
            *but other wifi card, wireless protocol stack in the wl driver, so we can call the bottom 
           *driver send out. Now, all the pkg that mirror to wifi 2.4G convert to multicast mac
           *address if set the flag: is_use_mcast_macaddr
           */
            if (netDev->is_use_mcast_macaddr == TRUE)
            {
                struct ethhdr * eth_hdr;
                eth_hdr = (struct ethhdr *)skbCopy->data;
                eth_hdr->h_dest[0] = 0x1;/*mcast mac address*/
                eth_hdr->h_dest[1] = 0x0;
                eth_hdr->h_dest[2] = 0x5E;
            }

            local_irq_save(flags);
            local_irq_enable();
            dev_queue_xmit(skbCopy);
            local_irq_restore(flags);
        }
    }
} /* MirrorPacket */

static void AEI_MultiMirrorPacket( struct sk_buff *skb, uint16_t mirFlags, bool mirrorIn )
{
    int i;
    uint32_t flag;

    if (mirFlags == 0)
        return;

    for (i = 0; pMirrorIntfNames[i]; i++)
    {
        flag = 0x0001 << i;

        if (!(mirFlags & flag))
            continue;

        AEI_MirrorPacket( skb, pMirrorIntfNames[i], mirrorIn);
    }
}
#endif /* AEI_VDSL_TOOLBOX */


static char *ethaddr = NULL;
module_param(ethaddr, charp, S_IRUGO);
MODULE_PARM_DESC(store, "ethaddr");

#ifdef RX_IP_HDR_REALIGN
static uint32_t rx_pkt_align = 0;
module_param(rx_pkt_align, uint, 0644);
MODULE_PARM_DESC(rx_pkt_align, "RX Pakcet IP header realign to 4byte boundary");
#endif

/* Alignment helper functions */
__always_inline static unsigned long align_up_off(unsigned long val, unsigned long step)
{
	return (((val + (step - 1)) & (~(step - 1))) - val);
}

__always_inline static unsigned long align_down_off(unsigned long val, unsigned long step)
{
	return ((val) & ((step) - 1));
}

__always_inline static unsigned long align_val_up(unsigned long val, unsigned long step)
{
	return ((val + step - 1) & (~(step - 1)));
}

__always_inline static unsigned long align_val_down(unsigned long val, unsigned long step)
{
	return (val & (~(step - 1)));
}

__always_inline static void* align_buf_dma(void *addr)
{
	return (void*)align_val_up((unsigned long)addr, dma_get_cache_alignment());
}

__always_inline static unsigned long align_buf_dma_offset(void *addr)
{
	return (align_buf_dma(addr) - addr);
}

__always_inline static void* align_buf_cache(void *addr)
{
	return (void*)align_val_down((unsigned long)addr, dma_get_cache_alignment());
}

__always_inline static unsigned long align_buf_cache_offset(void *addr)
{
	return (addr - align_buf_cache(addr));
}

__always_inline static unsigned long align_buf_cache_size(void *addr, unsigned long size)
{
	return align_val_up(size + align_buf_cache_offset(addr), dma_get_cache_alignment());
}

/* Print the Tx Request Queue */
static int txbd2str_range(struct vmac_priv *vmp, uint16_t s, int num)
{
	qdpc_pcie_bda_t *bda = vmp->bda;
	int i;

	printk("RC insert start index\t: %d\n", vmp->tx_bd_index);
	printk("RC reclaim start index\t: %d\n", vmp->tx_reclaim_start);
	printk("valid entries\t\t: %d\n", vmp->vmac_tx_queue_len);
	printk("Pkt index EP handled\t: %d\n", le32_to_cpu(VMAC_REG_READ(vmp->ep_next_rx_pkt)));

	printk("\t\t%8s\t%8s\t%8s\t%10s\n", "Address", "Valid", "Length", "Pkt Addr");

	for (i = 0; i < num; i++) {
		printk("\t%d\t0x%08x\t%8s\t\t%d\t0x%p\n", s, bda->request[s].addr, \
			(bda->request[s].info & PCIE_TX_VALID_PKT) ? "Valid" : "Invalid",  \
			bda->request[s].info & 0xffff, vmp->tx_skb[s]);
		VMAC_INDX_INC(s, vmp->tx_bd_num);
	}

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)

    //zli add debug
    printk("--------------------------Current Index:%d---------------------------\n", vmp->infowindex);
    printk("index\trqaddraddr\trqinfoaddr\trqindex\trqaddr  \trqvalue\n");
    for(i=0;i<RECORD_LENGTH_ZLI;i++)
        printk("%3d\t%8x\t%8x\t%4d\t%8x\t%8x\t\n", i,
               vmp->rqinfolist[i].rqaddraddr,
               vmp->rqinfolist[i].rqinfoaddr,
               vmp->rqinfolist[i].rqindex,
               vmp->rqinfolist[i].rqaddr,
               vmp->rqinfolist[i].rqvalue);
#endif

	return 0;
}

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
/*work around for rc rewrite dma entry*/
static int aei_rcRewrite(struct vmac_priv *vmp)
{
	qdpc_pcie_bda_t *bda = vmp->bda;
	int s = vmp->tx_reclaim_start;

#if 0
    printk("-----------------------AEI Current Index:%d---------------------------\n", vmp->infowindex);
	printk("RC insert start index\t: %d\n", vmp->tx_bd_index);
	printk("RC reclaim start index\t: %d\n", vmp->tx_reclaim_start);
	printk("valid entries\t\t: %d\n", vmp->vmac_tx_queue_len);
	printk("Pkt index EP handled\t: %d\n", le32_to_cpu(VMAC_REG_READ(vmp->ep_next_rx_pkt)));
#endif
	//printk("\t\t%8s\t%8s\t%8s\t%10s\n", "Address", "Valid", "Length", "Pkt Addr");

    if ( !(bda->request[s].info & PCIE_TX_VALID_PKT) ){
		trigger_flag++;
	}else{
		trigger_flag = 0;
    }
	
    if (trigger_flag > trigger_count){

        for (s = vmp->tx_reclaim_start; s < vmp->tx_bd_num ; s++) {

           if ( !(bda->request[s].info & PCIE_TX_VALID_PKT) )
           {
              
              printk("\t%d\t0x%08x\t%8s\t\t%d\t0x%p\n", 
                 s, bda->request[s].addr, \
                 (bda->request[s].info & PCIE_TX_VALID_PKT) ? "Valid" : "Invalid",  \
                 bda->request[s].info & 0xffff, vmp->tx_skb[s]);
              write_request_queue_by_roundindex(vmp, s);   
           }

        }

        for (s=0; s<vmp->tx_reclaim_start; s++){
            if ( !(bda->request[s].info & PCIE_TX_VALID_PKT) )
            {
               
               printk("\t%d\t0x%08x\t%8s\t\t%d\t0x%p\n", 
                  s, bda->request[s].addr, \
                  (bda->request[s].info & PCIE_TX_VALID_PKT) ? "Valid" : "Invalid",  \
                  bda->request[s].info & 0xffff, vmp->tx_skb[s]);
               write_request_queue_by_roundindex(vmp, s);   
            }
        }
        printk("rc rewrite triggered count %d, trigger_flag %d\n",++rewrite,trigger_flag);
        ipc_trigger_manually(vmp);
        trigger_flag = 0;
        
    }
#if 0    
    //Scott add debug
    printk("-----------------------AEI Current Index:%d---------------------------\n", vmp->infowindex);
    printk("index\trqaddraddr\trqinfoaddr\trqindex\trqaddr  \trqvalue\n");
    for(i=0;i<RECORD_LENGTH_ZLI;i++)
        printk("%3d\t%8x\t%8x\t%4d\t%8x\t%8x\t\n", i,
               vmp->rqinfolist[i].rqaddraddr,
               vmp->rqinfolist[i].rqinfoaddr,
               vmp->rqinfolist[i].rqindex,
               vmp->rqinfolist[i].rqaddr,
               vmp->rqinfolist[i].rqvalue);
#endif

	return 0;
}
#endif
static int txbd2str(struct vmac_priv *vmp)
{
	uint16_t s;

	s = VMAC_INDX_MINUS(vmp->tx_bd_index, 4, vmp->tx_bd_num);
	return txbd2str_range(vmp, s, 8);
}

static int txbd2str_all(struct vmac_priv *vmp)
{
	return txbd2str_range(vmp, 0, vmp->tx_bd_num);
}

static int rxbd2str_range(struct vmac_priv *vmp, uint16_t s, int num)
{
	int i;
	char *idxflg;

	printk("rxindx\trbdaddr\t\tbuff\t\tinfo\t\trx_skb\n");
	for (i = 0; i < num; i++) {
		if(s == vmp->rx_bd_index)
			idxflg = ">rbd";
		else
			idxflg = "";
		printk("%2d%s\t@%p\t%08x\t%08x\t%p\n", s, idxflg,
			&vmp->rx_bd_base[s], vmp->rx_bd_base[s].buff_addr,
			vmp->rx_bd_base[s].buff_info, vmp->rx_skb[s]);

		VMAC_INDX_INC(s, vmp->rx_bd_num);
	}
	return 0;
}

static int rxbd2str(struct vmac_priv *vmp)
{
	uint16_t s;
	s = VMAC_INDX_MINUS(vmp->rx_bd_index, 4, vmp->rx_bd_num);
	return rxbd2str_range(vmp, s, 8);
}

static int rxbd2str_all(struct vmac_priv *vmp)
{
	return rxbd2str_range(vmp, 0, vmp->rx_bd_num);
}

static int vmaccnt2str(struct vmac_priv *vmp, char *buff)
{
	int count;
	count = sprintf(buff, "tx_bd_busy_cnt:\t%08x\n", vmp->tx_bd_busy_cnt);
	count += sprintf(buff + count, "tx_stop_queue_cnt:\t%08x\n", vmp->tx_stop_queue_cnt);
	count += sprintf(buff + count, "rx_skb_alloc_failures:\t%08x\n", vmp->rx_skb_alloc_failures);
	count += sprintf(buff + count, "intr_cnt:\t%08x\n", vmp->intr_cnt);
	count += sprintf(buff + count, "vmac_xmit_cnt:\t%08x\n", vmp->vmac_xmit_cnt);
	count += sprintf(buff + count, "vmac_skb_free:\t%08x\n", vmp->vmac_skb_free);
#ifdef QTN_SKB_RECYCLE_SUPPORT
	count += sprintf(buff + count, "skb_recycle_cnt:\t%08x\n", vmp->skb_recycle_cnt);
	count += sprintf(buff + count, "skb_recycle_failures:\t%08x\n", vmp->skb_recycle_failures);
#endif
	count += sprintf(buff + count, "vmp->txqueue_stopped=%x\n", vmp->txqueue_stopped);
	count += sprintf(buff + count, "*vmp->txqueue_wake=%x\n", *vmp->txqueue_wake);
#ifdef RX_IP_HDR_REALIGN
	if(rx_pkt_align)
		count += sprintf(buff + count, "rx iphdr aligned:%d,unalign:%d\n", align_cnt, unalign_cnt);
#endif
	return count;
}

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)

//zli add debug
static void  write_request_queue_by_roundindex(struct vmac_priv *vmp, uint32_t index )
{
	printk("zli debug, write queue triggered, index:%u, address:%x, value: %x \n", index, 
		vmp->rqinfolist[index].rqaddr, vmp->rqinfolist[index].rqvalue );
	VMAC_REG_WRITE(vmp->rqinfolist[index].rqaddraddr, vmp->rqinfolist[index].rqaddr);
	VMAC_REG_WRITE(vmp->rqinfolist[index].rqinfoaddr, vmp->rqinfolist[index].rqvalue); 
}
static void ipc_trigger_manually(struct vmac_priv *vmp)
{
	printk("zli debug, IPC triggered\n");
	writel(TOPAZ_SET_INT(IPC_EP_RX_PKT), vmp->ep_ipc_reg);
}

#endif

static ssize_t vmac_dbg_show(struct device *dev, struct device_attribute *attr,
						char *buff)
{
	struct net_device *ndev = container_of(dev, struct net_device, dev);
	struct vmac_priv *vmp = netdev_priv(ndev);
	int count = 0;
	switch (vmp->show_item) {
	case SHOW_TX_BD: /* Print Tx Rquest Queue */
		count = (ssize_t)txbd2str_all(vmp);
        break;		
#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
	case IPC_TRIGGER_MANUALLY:
		ipc_trigger_manually(vmp);		
		break;
	case READ_TRIGGER_FLAG:
	    printk("trigger_flag: %d,same_idx_cnt: %d,prev_end_idx: %d\n",trigger_flag,same_idx_cnt,prev_end_idx);
	    break;
	case SET_TRIGGER_COUNT:
	    printk("trigger_count: %d, rewrite: %d\n",trigger_count, rewrite);  
	    break;
#endif	    
	case SHOW_RX_BD:/* show Rx BD */
		count = (ssize_t)rxbd2str_all(vmp);
		break;
	case SHOW_VMAC_STATS:/* show vmac interrupt statistic info */
		count = vmaccnt2str(vmp, buff);
	default:
		break;
	}
	return count;
}

static ssize_t vmac_dbg_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = container_of(dev, struct net_device, dev);
	struct vmac_priv *vmp = netdev_priv(ndev);
#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
	uint16_t cmd;
	cmd = (uint16_t)simple_strtoul(buf, NULL, 10);
	printk("zli debug, cmd:%d\n", cmd);
#else
    uint8_t cmd;
    cmd = (uint8_t)simple_strtoul(buf, NULL, 10);
#endif
	if (cmd < 16) {
		switch(cmd) {
		case 0:
			vmp->dbg_flg = 0; /* disable all of runtime dump */
			break;
		case 1:
			napi_schedule(&vmp->napi);
			break;
		case 2:
			vmp->tx_bd_busy_cnt = 0;
			vmp->intr_cnt = 0;
			vmp->rx_skb_alloc_failures = 0;
#ifdef GPL_CODE_DEBUG_QTN_RELOAD
    case 3:
        aei_qtn_reload_flag = 1;
        break;
#endif
		default:
			break;
		}
	}
	else if (cmd < 32) /* used for vmac_dbg_show */
		vmp->show_item = cmd;
	else if (cmd < 64) /* used for runtime dump */
		vmp->dbg_flg |= (0x1 << (cmd - 32));
	else if (cmd == 64) /* enable all of runtime dump */
		vmp->dbg_flg = -1;
#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)		
    //zli add debug
    //>=1000 && < 2100 -- rewrite request queue by index=(cmd-1000)
    else if (cmd >=1000 && cmd < (1000+RECORD_LENGTH_ZLI))
       write_request_queue_by_roundindex(vmp, (cmd-1000));
#endif
	return count;
}
static DEVICE_ATTR(dbg, S_IWUSR | S_IRUSR, vmac_dbg_show, vmac_dbg_set); /* dev_attr_dbg */

static ssize_t vmac_pm_show(struct device *dev, struct device_attribute *attr,
						char *buff)
{
	struct net_device *ndev = container_of(dev, struct net_device, dev);
	struct vmac_priv *vmp = netdev_priv(ndev);
	int count = 0;

	count += sprintf(buff + count, "PCIE Device Power State : %s\n",
				le32_to_cpu(*vmp->ep_pmstate) == PCI_D3hot ? "D3" : "D0");

	return count;
}

static ssize_t vmac_pm_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = container_of(dev, struct net_device, dev);
	struct vmac_priv *vmp = netdev_priv(ndev);
	uint8_t cmd;

	cmd = (uint8_t)simple_strtoul(buf, NULL, 10);

	if (cmd == 0) {
		qdpc_pcie_resume(vmp->pdev);
	} else if (cmd == 1) {
		pm_message_t state;
		state.event = 0;
		qdpc_pcie_suspend(vmp->pdev, state);
	}

	return count;
}
static DEVICE_ATTR(pmctrl, S_IWUSR | S_IRUSR, vmac_pm_show, vmac_pm_set); /* dev_attr_pmctrl */

static struct attribute *vmac_device_attrs[] = {
	&dev_attr_dbg.attr,
	&dev_attr_pmctrl.attr,
	NULL,
};

static const struct attribute_group vmac_attr_group = {
	.attrs = vmac_device_attrs,
};

#ifdef VMAC_DEBUG_MODE
static void dump_pkt(char *data, int len, char *s)
{
	int i;

	if (len > 128)
		len = 128;
	printk("%spkt start%p>\n", s, data);
	for (i = 0; i < len;) {
		printk("%02x ", data[i]);
		if ((++i % 16) == 0)
			printk("\n");
	}
	printk("<%spkt end\n", s);
}

static void dump_rx_interrupt(struct vmac_priv *vmp)
{
	printk("intr_cnt:\t%08x\n", vmp->intr_cnt);
}
#endif

#define VMAC_BD_INT32_VAR 3

#if defined(GPL_CODE)
static uint32_t paddr_save;
#endif

static int alloc_bd_tbl(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	uint32_t ucaddr;
	uint32_t paddr;
	int len;	/* Length of allocated Transmitted & Received descriptor array */

	/* uint32_t is used to be updated by ep */
	len = (vmp->tx_bd_num + vmp->rx_bd_num) * VMAC_BD_LEN + VMAC_BD_INT32_VAR * sizeof(uint32_t);
	ucaddr = (uint32_t)pci_alloc_consistent(vmp->pdev, len, (dma_addr_t *)&paddr);
	if (!ucaddr)
		return -1;
#if defined(GPL_CODE)
	paddr_save = paddr;
#endif
	memset((void *)ucaddr, 0, len);

	vmp->addr_uncache = ucaddr;
	vmp->uncache_len = len;

	/* Update pointers related with Tx descriptor table */
	vmp->tx_bd_base = (struct vmac_bd *)ucaddr;
	qdpc_pcie_posted_write(paddr, &vmp->bda->bda_rc_tx_bd_base);
	init_tx_bd(vmp);
	printk("Tx Descriptor table: uncache virtual addr: 0x%08x paddr: 0x%08x\n",
		(uint32_t)vmp->tx_bd_base, paddr);

	/* Update pointers related with Rx descriptor table */
	ucaddr += vmp->tx_bd_num * VMAC_BD_LEN;
	paddr += vmp->tx_bd_num * VMAC_BD_LEN;

	vmp->rx_bd_base = (struct vmac_bd *)ucaddr;
	qdpc_pcie_posted_write(paddr, &vmp->bda->bda_rc_rx_bd_base);
	printk("Rx Descriptor table: uncache virtual addr: 0x%08x paddr: 0x%08x\n",
		(uint32_t)vmp->rx_bd_base, paddr);

	/* Update pointers used by EP's updating consumed packet index */
	ucaddr += vmp->rx_bd_num * VMAC_BD_LEN;
	paddr += vmp->rx_bd_num * VMAC_BD_LEN;

	vmp->ep_next_rx_pkt = (uint32_t *)ucaddr;
	qdpc_pcie_posted_write(paddr, &vmp->bda->bda_ep_next_pkt);
	printk("EP_handled_idx: uncache virtual addr: 0x%08x paddr: 0x%08x\n",
		(uint32_t)vmp->ep_next_rx_pkt, paddr);

	ucaddr += sizeof(uint32_t);
	paddr += sizeof(uint32_t);

	vmp->txqueue_wake = (uint32_t *)ucaddr;

	ucaddr += sizeof(uint32_t);
	paddr += sizeof(uint32_t);
	vmp->ep_pmstate = (uint32_t *)ucaddr;

	return 0;
}

static void free_bd_tbl(struct vmac_priv *vmp)
{
	pci_free_consistent(vmp->pdev, vmp->uncache_len, (void *)vmp->addr_uncache,
#if defined(GPL_CODE)
	/*
	 * QTN wrote the allocated PCI physical address into vmp->bda->bda_rc_tx_bd_base.
	 * But when EP reset due to unknown hardware/software bug,
	 * qdpc_pci_readl(&vmp->bda->bda_rc_tx_bd_base) always return 0xffffffff
	 *
	 * So we have to save this physical address and pass it to pci_free_consistent().
	 */
			paddr_save);
#else
			qdpc_pci_readl(&vmp->bda->bda_rc_tx_bd_base));
#endif
}

static int alloc_skb_desc_array(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	uint32_t addr;
	int len;

	len = (vmp->tx_bd_num + vmp->rx_bd_num) * (sizeof(struct sk_buff *));
	addr = (uint32_t)kzalloc(len, GFP_KERNEL);
	if (!addr)
		return -1;
	vmp->tx_skb = (struct sk_buff **)addr;

	addr += vmp->tx_bd_num * sizeof(struct sk_buff *);
	vmp->rx_skb = (struct sk_buff **)addr;

	return 0;
}

static void free_skb_desc_array(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);

	kfree(vmp->tx_skb);
}

#ifdef QTN_SKB_RECYCLE_SUPPORT
static inline struct sk_buff *__vmac_rx_skb_freelist_pop(struct vmac_priv *vmp)
{
	struct sk_buff *skb = __skb_dequeue(&vmp->rx_skb_freelist);

	return skb;
}

static inline int vmac_rx_skb_freelist_push(struct vmac_priv *vmp, dma_addr_t buff_addr, struct sk_buff *skb)
{
	unsigned long flag;

	if (skb_queue_len(&vmp->rx_skb_freelist) > QTN_RX_SKB_FREELIST_MAX_SIZE) {
		pci_unmap_single(vmp->pdev, buff_addr, skb->len, (int)DMA_BIDIRECTIONAL);
		dev_kfree_skb(skb);
		vmp->vmac_skb_free++;
		return 0;
	}

	/* check for undersize skb; this should never happen, and indicates problems elsewhere */
	if (unlikely((skb_end_pointer(skb) - skb->head) < QTN_RX_BUF_MIN_SIZE)) {
		pci_unmap_single(vmp->pdev, buff_addr, skb->len, (int)DMA_BIDIRECTIONAL);
		dev_kfree_skb(skb);
		vmp->vmac_skb_free++;
		vmp->skb_recycle_failures++;
		return -EINVAL;
	}

	skb->len = 0;
	skb->tail = skb->data = skb->head;
	skb_reserve(skb, NET_SKB_PAD);
	skb_reserve(skb, align_buf_dma_offset(skb->data));

	qtn_spin_lock_bh_save(&vmp->rx_skb_freelist_lock, &flag);
	__skb_queue_tail(&vmp->rx_skb_freelist, skb);
	qtn_spin_unlock_bh_restore(&vmp->rx_skb_freelist_lock, &flag);

	vmp->skb_recycle_cnt++;

	return 0;
}

static inline void __vmac_rx_skb_freelist_refill(struct vmac_priv *vmp)
{
	struct sk_buff *skb = NULL;
	int num = vmp->rx_skb_freelist_fill_level - skb_queue_len(&vmp->rx_skb_freelist);

	while (num > 0) {
		if (!(skb = dev_alloc_skb(SKB_BUF_SIZE))) {
			vmp->rx_skb_alloc_failures++;
			break;
		}
		/* Move skb->data to a cache line boundary */
		skb_reserve(skb, align_buf_dma_offset(skb->data));
		pci_map_single(vmp->pdev, skb->data, skb_end_pointer(skb) - skb->data, (int)DMA_FROM_DEVICE);
		__skb_queue_tail(&vmp->rx_skb_freelist, skb);

		num--;
	}
}

static void vmac_rx_skb_freelist_purge(struct vmac_priv *vmp)
{
	unsigned long flag;

	qtn_spin_lock_bh_save(&vmp->rx_skb_freelist_lock, &flag);
	__skb_queue_purge(&vmp->rx_skb_freelist);
	qtn_spin_unlock_bh_restore(&vmp->rx_skb_freelist_lock, &flag);
}
#endif /* QTN_SKB_RECYCLE_SUPPORT */

static inline bool check_netlink_magic(qdpc_cmd_hdr_t *cmd_hdr)
{
	return ((memcmp(cmd_hdr->dst_magic, QDPC_NETLINK_DST_MAGIC, ETH_ALEN) == 0)
		&& (memcmp(cmd_hdr->src_magic, QDPC_NETLINK_SRC_MAGIC, ETH_ALEN) == 0));
}

static void vmac_netlink_rx(struct net_device *ndev, void *buf, size_t len, int rpc_type)
{
	struct vmac_priv *priv = netdev_priv(ndev);
	struct sk_buff *skb = nlmsg_new(len, 0);
	struct nlmsghdr *nlh;
	int pid = 0;

	if (skb == NULL) {
		DBGPRINTF("WARNING: out of netlink SKBs\n");
		return;
	}

	nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, len, 0);  ;
	memcpy(nlmsg_data(nlh), buf, len);
	NETLINK_CB(skb).dst_group = 0;

		if (rpc_type == QDPC_RPC_TYPE_STRCALL)
		pid = priv->str_call_nl_pid;
	else if (rpc_type == QDPC_RPC_TYPE_LIBCALL)
		pid = priv->lib_call_nl_pid;

	if (unlikely(pid == 0)) {
		kfree_skb(skb);
		return;
	}

	nlmsg_unicast(priv->nl_socket, skb, pid);
}

static inline void vmac_napi_schedule(struct vmac_priv *vmp)
{
	if (napi_schedule_prep(&vmp->napi)) {
		disable_vmac_ints(vmp);
		__napi_schedule(&vmp->napi);
	}
}

#ifdef QDPC_PLATFORM_IRQ_FIXUP
static inline int vmac_has_more_rx(struct vmac_priv *vmp)
{
	uint16_t i = vmp->rx_bd_index;
	volatile struct vmac_bd *rbdp = &vmp->rx_bd_base[i];

	return !(le32_to_cpu(rbdp->buff_info) & VMAC_BD_EMPTY);
}
static inline void vmac_irq_open_fixup(struct vmac_priv *vmp)
{
	vmac_napi_schedule(vmp);
}
/*
 * TODO: vmac_irq_napi_fixup needs to undergo stability and
 * especially performance test to justify its value
*/
static inline void vmac_irq_napi_fixup(struct vmac_priv *vmp)
{
	if (unlikely(vmac_has_more_rx(vmp)))
		vmac_napi_schedule(vmp);
}
#else
#define vmac_irq_open_fixup(v) do{}while(0)
#define vmac_irq_napi_fixup(v) do{}while(0)
#endif


#ifdef RX_IP_HDR_REALIGN
/*
 * skb buffer have a pading, so skb data move less than pading is safe
 *
 */
static void vmac_rx_ip_align_ahead(struct sk_buff *skb, uint32_t move_bytes)
{
	uint8_t *pkt_src, *pkt_dst;
	uint8_t bytes_boundary = ((uint32_t)(skb->data)) % 4;
	BUG_ON(bytes_boundary & 1);

	/*bytes_boundary == 0 means etherheader is 4 byte aligned,
	 *so IP header is 2(+14 ether header) byte aligned,
	 *move whole packet 2 byte ahead for QCA NSS preference
	*/

	if(bytes_boundary == 0){
		if(skb_headroom(skb) >= move_bytes){
			pkt_src = skb->data;
			pkt_dst = skb->data - move_bytes;

			memmove(pkt_dst, pkt_src, skb->len);

			skb->data -= move_bytes;
			skb->tail -= move_bytes;
		}
		unalign_cnt++;
	}
	else if(bytes_boundary == 2){
		align_cnt++;
	}
}
#endif

#if defined(GPL_CODE_CPU_CACHE_OPTIMIZATION)
/* return physical address of the virtual address */
static uint32_t aei_get_phy_addr(void *cpu_addr)
{
	unsigned long offset;
	struct page *page;

	page = virt_to_page(cpu_addr);
	offset = (unsigned long)cpu_addr & ~PAGE_MASK;
	return pfn_to_dma(NULL, page_to_pfn(page)) + offset;
}

static void aei_invalidate_cpu_cache(uint32_t paddr, int size)
{
	/*
	 * outer_inv_range() is broadcom 963138 specific function.
	 * Need to call similar function on other platform.
	 */
	outer_inv_range(paddr, paddr + size);
}
#endif

static int __sram_text vmac_rx_poll(struct napi_struct *napi, int budget)
{
	struct vmac_priv *vmp = container_of(napi, struct vmac_priv, napi);
	struct net_device *ndev = vmp->ndev;
	struct ethhdr *eth;
	qdpc_cmd_hdr_t *cmd_hdr;
	int processed = 0;
	uint16_t i = vmp->rx_bd_index;
	volatile struct vmac_bd *rbdp = &vmp->rx_bd_base[i];
	uint32_t descw1;

	while (!((descw1 = le32_to_cpu(VMAC_REG_READ(&rbdp->buff_info))) & VMAC_BD_EMPTY) && (processed < budget)) {
		struct sk_buff *skb;
		skb = vmp->rx_skb[i];
		if (skb) {
			skb_reserve(skb, VMAC_GET_OFFSET(descw1));
			skb_put(skb, VMAC_GET_LEN(descw1));

			eth = (struct ethhdr *)(skb->data);
			if (unlikely(ntohs(eth->h_proto) == QDPC_APP_NETLINK_TYPE)) {
				/* Double Check if it's netlink packet*/
				cmd_hdr = (qdpc_cmd_hdr_t *)skb->data;
				if (check_netlink_magic(cmd_hdr)) {
					vmac_netlink_rx(ndev,
						skb->data + sizeof(qdpc_cmd_hdr_t),
						ntohs(cmd_hdr->len) - sizeof(cmd_hdr->rpc_type),
						ntohs(cmd_hdr->rpc_type));
				}
				dev_kfree_skb(skb);
			} else {
#if defined(GPL_CODE_TOOLBOX)
				struct sk_buff *tmp_skb = skb;
				struct ethhdr *eth;

				eth = (struct ethhdr *)tmp_skb->data;

				if (vmp->ndev && vmp->usMirrorInFlags != 0/* &&
				    eth->h_proto != ETHER_TYPE_BRCM*/)//fixme
				{
					AEI_MultiMirrorPacket( tmp_skb , vmp->usMirrorInFlags, TRUE);
				}
#endif /* AEI_VDSL_TOOLBOX */

#if !defined(GPL_CODE_CPU_CACHE_OPTIMIZATION)
#ifdef QTN_SKB_RECYCLE_SUPPORT
				pci_unmap_single(vmp->pdev, rbdp->buff_addr,
					skb_end_pointer(skb) - skb->data, (int)DMA_BIDIRECTIONAL);
#else
				pci_unmap_single(vmp->pdev, rbdp->buff_addr,
					skb_end_pointer(skb) - skb->data, (int)DMA_FROM_DEVICE);
#endif /* QTN_SKB_RECYCLE_SUPPORT */
#endif

#ifdef RX_IP_HDR_REALIGN
				if (rx_pkt_align)
					vmac_rx_ip_align_ahead(skb, 2);
#endif
				dump_rx_pkt(vmp, (char *)skb->data, (int)skb->len);

				processed++;

                                #if defined(GPL_CODE)
                                if (blog_sinit(skb, skb->dev, TYPE_ETH, 0, BLOG_EXTRA1PHY) != PKT_DONE) {
                                        skb->protocol = eth_type_trans(skb, ndev);
                                        netif_receive_skb(skb);
                                }
                                #else
                                skb->protocol = eth_type_trans(skb, ndev);
                                netif_receive_skb(skb);
                                #endif


				ndev->stats.rx_packets++;
				ndev->stats.rx_bytes += VMAC_GET_LEN(descw1);
			}
		}
		if ((ndev->stats.rx_packets & RX_DONE_INTR_MSK) == 0)
			writel(TOPAZ_SET_INT(IPC_RC_RX_DONE), vmp->ep_ipc_reg);

		dump_rx_bd(vmp);

		ndev->last_rx = jiffies;

		/*
		 * We are done with the current buffer attached to this descriptor, so attach a new
		 * one.
		 */
		if (skb2rbd_attach(ndev, i, descw1 & VMAC_BD_WRAP) == 0) {
			if (++i >= vmp->rx_bd_num)
				i = 0;
			vmp->rx_bd_index = i;
			rbdp = &vmp->rx_bd_base[i];
		} else {
			break;
		}
	}
#ifdef QTN_WAKEQ_SUPPORT
	vmac_try_wake_queue(ndev);
#endif
	if (processed < budget) {
		napi_complete(napi);
		enable_vmac_ints(vmp);
		vmac_irq_napi_fixup(vmp);
	}

#ifdef QTN_SKB_RECYCLE_SUPPORT
	spin_lock(&vmp->rx_skb_freelist_lock);
	__vmac_rx_skb_freelist_refill(vmp);
	spin_unlock(&vmp->rx_skb_freelist_lock);
#endif

	return processed;
}

#if defined(GPL_CODE_SKB_POOL)
extern struct sk_buff *aei_get_skb(int size);
#endif

static int __sram_text skb2rbd_attach(struct net_device *ndev, uint16_t rx_bd_index, uint32_t wrap)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	volatile struct vmac_bd * rbdp;
	uint32_t buff_addr;
	struct sk_buff *skb = NULL;
#if defined(GPL_CODE_SKB_POOL)
        if (!(skb = aei_get_skb(SKB_BUF_SIZE))) {
		vmp->rx_skb_alloc_failures++;
		vmp->rx_skb[rx_bd_index] = NULL;/* prevent old packet from passing the packet up */
		return -1;
	}
#else
#ifdef QTN_SKB_RECYCLE_SUPPORT
	spin_lock(&vmp->rx_skb_freelist_lock);
	if (unlikely(!(skb = __vmac_rx_skb_freelist_pop(vmp)))) {
		spin_unlock(&vmp->rx_skb_freelist_lock);
		vmp->rx_skb[rx_bd_index] = NULL;/* prevent old packet from passing the packet up */
		return -1;
	}
	spin_unlock(&vmp->rx_skb_freelist_lock);
#else
	if (!(skb = dev_alloc_skb(SKB_BUF_SIZE))) {
		vmp->rx_skb_alloc_failures++;
		vmp->rx_skb[rx_bd_index] = NULL;/* prevent old packet from passing the packet up */
		return -1;
	}
#endif /* QTN_SKB_RECYCLE_SUPPORT */
#endif
	skb->dev = ndev;

	vmp->rx_skb[rx_bd_index] = skb;

#if defined(GPL_CODE)
	/* Reserve head for byte-aligned  */
	skb_reserve(skb, BCM_PKT_HEADROOM);
#endif

#ifndef QTN_SKB_RECYCLE_SUPPORT
	/* Move skb->data to a cache line boundary */
	skb_reserve(skb, align_buf_dma_offset(skb->data));
#endif /* QTN_SKB_RECYCLE_SUPPORT */

	/* Invalidate cache and map virtual address to bus address. */
	rbdp = &vmp->rx_bd_base[rx_bd_index];
#if defined(GPL_CODE_CPU_CACHE_OPTIMIZATION)
 /*
  * This is the key point:
  * We don't need to invalidate CPU cache here.
  * Because only DMA hardware will write data to this buffer and DMA
  * don't care about CPU cache.
  * We will invalidate CPU cache in vmac_rx_poll() just before CPU
  * start to access the buffer to make sure that CPU won't get wrong
  * data from cache.
  */
	buff_addr = aei_get_phy_addr(skb->data);

	/*
	 * We used to invalidate CPU cache in vmac_rx_poll(),
	 * but it caused unstable TCP RX throughput.
	 *
	 * Move CPU cache invalidation here to fix it.
	 * But it's shamed that I'm not sure how it makes difference.
	 */
	aei_invalidate_cpu_cache(buff_addr, skb_end_pointer(skb) - skb->data);
#else
#ifdef QTN_SKB_RECYCLE_SUPPORT
	buff_addr = virt_to_bus(skb->data);
#else
	buff_addr = (uint32_t)pci_map_single(vmp->pdev, skb->data,
				skb_end_pointer(skb) - skb->data, (int)DMA_FROM_DEVICE);
#endif
#endif
	rbdp->buff_addr = cpu_to_le32(buff_addr);

	/* TODO: packet length, currently don't check the length */
	rbdp->buff_info =  cpu_to_le32(VMAC_BD_EMPTY | wrap);

	return 0;
}


static __attribute__((section(".sram.text"))) void
vmac_tx_teardown(struct net_device *ndev, qdpc_pcie_bda_t *bda)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	volatile struct vmac_bd *tbdp;
	uint16_t i;
	uint32_t end_idx = le32_to_cpu(VMAC_REG_READ(vmp->ep_next_rx_pkt));
	i = vmp->tx_reclaim_start;

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
     if ( (vmp->vmac_tx_queue_len == RECORD_LENGTH_ZLI-1) && (i == end_idx) ){
         if (prev_end_idx != end_idx)
         {
            prev_end_idx = end_idx;
            same_idx_cnt = 0;
         }
         else
         { 
            same_idx_cnt++;            
            //printk("aei: same_idx_cnt=%d,end_index=%d\n",same_idx_cnt,end_idx);
            if (same_idx_cnt==10) {
               same_idx_cnt = 0;
               aei_rcRewrite(vmp);
            }
         } 
     }    
#endif

	while (i != end_idx) {
		struct sk_buff *skb;
		tbdp = &vmp->tx_bd_base[i];
		skb = vmp->tx_skb[i];
		if (!skb)
			break;

		ndev->stats.tx_packets++;

		ndev->stats.tx_bytes +=  skb->len;
#if defined(GPL_CODE)
		pci_unmap_single(vmp->pdev, (dma_addr_t)tbdp->buff_addr,
			skb->len, (int)DMA_TO_DEVICE);
#else
#ifdef QTN_SKB_RECYCLE_SUPPORT
		vmac_rx_skb_freelist_push(vmp, (dma_addr_t)tbdp->buff_addr, skb);
#else
		pci_unmap_single(vmp->pdev, (dma_addr_t)tbdp->buff_addr,
			skb->len, (int)DMA_TO_DEVICE);
		dev_kfree_skb(skb);
#endif /* QTN_SKB_RECYCLE_SUPPORT */
#endif
		vmp->tx_skb[i] = NULL;

		vmp->vmac_skb_free++;

		vmp->vmac_tx_queue_len--;

		if (++i >= vmp->tx_bd_num)
			i = 0;
#if defined(GPL_CODE)
		/*
		 * dev_kfree_skb() calls bcm63xx_enet_recycle() which acquiring ethlock_rx.
		 * So below deadlock happened.
		 *
		 * aei_tx_lock protects vmp structure and it's accessed with lock held.
		 * Now it's OK to release aei_tx_lock and acquire it again to prevent deadlock.
		 *
		 * CPU 0                CPU 1
		 *
		 * ethlock_rx           aei_tx_lock
		 *
		 * aei_tx_lock          ethlock_rx
		 *
		 * 2014-10-23 Update:
		 *
		 * We used to unlock aei_tx_lock here to fix above deadlock.
		 * This is unsafe because it opens a small window that vmac_tx_teardown() is re-entered
		 * by another CPU, and vmp->tx_reclaim_start is read/write concurrently by 2 CPUs.
		 * This messes the number of free DMA TX entries.
		 *
		 * Now we just queue the skb here and will free it after vmac_tx() is done,
		 * so it's freed outside of aei_tx_lock. See vmac_xmit().
		 */

		skb_queue_head(&vmp->aei_teardown_queue, skb);
#endif
	}

	vmp->tx_reclaim_start = i;
}

#ifdef QTN_TX_SKBQ_SUPPORT
static inline int __vmac_process_tx_skbq(struct net_device *ndev, uint32_t budget)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	struct sk_buff *skb;

	while(!vmp->txqueue_stopped && (skb = __skb_dequeue(&vmp->tx_skb_queue)) != NULL) {
		if (vmac_tx((void *)skb, ndev) != NETDEV_TX_OK) {
			__skb_queue_head(&vmp->tx_skb_queue, skb);
			break;
		}

		if (--budget == 0) {
			break;
		}
	}

	if (skb_queue_len(&vmp->tx_skb_queue) && !vmp->txqueue_stopped) {
		tasklet_schedule(&vmp->tx_skbq_tasklet);
	}

	return NETDEV_TX_OK;
}

static int vmac_process_tx_skbq(struct net_device *ndev, uint32_t budget)
{
	int ret;
	struct vmac_priv *vmp = netdev_priv(ndev);

	spin_lock(&vmp->tx_skbq_lock);
	ret = __vmac_process_tx_skbq(ndev, budget);
	spin_unlock(&vmp->tx_skbq_lock);

	return ret;
}

static void __attribute__((section(".sram.text"))) vmac_tx_skbq_tasklet(unsigned long data)
{
	struct net_device *ndev = (struct net_device *)data;
	struct vmac_priv *vmp = netdev_priv(ndev);

	vmac_process_tx_skbq(ndev, vmp->tx_skbq_tasklet_budget);
}

static int vmac_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	int ret;
	unsigned long flag;

	if (unlikely(skb_queue_len(&vmp->tx_skb_queue) >= vmp->tx_skbq_max_size)) {
                dev_kfree_skb((void *)skb);
		return NETDEV_TX_OK;
	}

	qtn_spin_lock_bh_save(&vmp->tx_skbq_lock, &flag);
	__skb_queue_tail(&vmp->tx_skb_queue, skb);
	ret = __vmac_process_tx_skbq(ndev, vmp->tx_skbq_budget);
	qtn_spin_unlock_bh_restore(&vmp->tx_skbq_lock, &flag);

	return ret;
}
#else
static int vmac_xmit(struct sk_buff *skb, struct net_device *ndev)
{
#if defined(GPL_CODE)
        struct vmac_priv *vmp = netdev_priv(ndev);
        struct sk_buff *orig_skb = skb;
        int ret;

        skb = nbuff_xlate((pNBuff_t)skb);
        if (skb == NULL)
        {
                nbuff_free((pNBuff_t)orig_skb);
                return 0;
        }
        blog_emit(skb, ndev, TYPE_ETH, 0, BLOG_EXTRA1PHY);

        /*
         * If packets are forwarded from Eth to 5G wifi,
         * then this function always runs on CPU0, because NSS interrupt
         * is binded to CPU0.
         *
         * But if packets are sent out from DUT, then this function may run
         * on CPU1 concurrently. So we need lock to protect:
         *  - skb free (see vmac_tx_teardown())
         *  - modification of vmac_priv
         *  - concurrent access to TX DMA
         *
         *  Don't need to disable bottom half(spin_lock_bh), both two paths:
         *  packet forwarded from BCM ethernet or send locally from userspace application,
         *  are already in softirq context.
         */
        spin_lock(&vmp->aei_tx_lock);
        ret = vmac_tx((void *)skb, ndev);
        spin_unlock(&vmp->aei_tx_lock);

	/*
	 * Free the skb outside of aei_tx_lock
	 */

	while ((skb = skb_dequeue(&vmp->aei_teardown_queue))) {
		/*
		 * If this is connection track skb,
		 * __kfree_skb() ->
		 *  skb_release_head_state() ->
		 *  nf_conntrack_destroy() ->
		 *  destroy_conntrack() ->
		 *  blog_notify(), which will acquire blog_lock.
		 *
		 *  And if we are in flow cache path which already held blog_lock,
		 *  then deadlock will happen.
		 *
		 *  So we delay free of connection track skb to skbFreeTask to avoid the deadlock.
		 *  Other skbs are still directly freed in flow cache path.
		 */
		if (skb->nfct)
			dev_kfree_skb_thread(skb);
		else
			dev_kfree_skb(skb);
	}

        return ret;
#else
	return vmac_tx((void *)skb, ndev);
#endif
}
#endif

void vmac_tx_drop(void *pkt_handle, struct net_device *ndev)
{
	struct sk_buff *skb;

	{
                skb = (struct sk_buff *)pkt_handle;
                dev_kfree_skb((void *)skb);
        }
}

#ifdef QTN_WAKEQ_SUPPORT
static inline void vmac_try_stop_queue(struct net_device *ndev)
{
	unsigned long flags;
	struct vmac_priv *vmp = netdev_priv(ndev);

	spin_lock_irqsave(&vmp->txqueue_op_lock, flags);

	if (!vmp->txqueue_stopped) {
		vmp->txqueue_stopped = 1;
		*vmp->txqueue_wake = 0;
		barrier();
		writel(TOPAZ_SET_INT(IPC_RC_STOP_TX), vmp->ep_ipc_reg);
		vmp->tx_stop_queue_cnt++;
		netif_stop_queue(ndev);
	}
	spin_unlock_irqrestore(&vmp->txqueue_op_lock, flags);
}

static inline void vmac_try_wake_queue(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	unsigned long flags;

	spin_lock_irqsave(&vmp->txqueue_op_lock, flags);
	if (vmp->txqueue_stopped && *vmp->txqueue_wake) {

		vmp->txqueue_stopped = 0;

		netif_wake_queue(ndev);
#ifdef QTN_TX_SKBQ_SUPPORT
		tasklet_schedule(&vmp->tx_skbq_tasklet);
#endif
	}
	spin_unlock_irqrestore(&vmp->txqueue_op_lock, flags);
}
#endif

int __attribute__((section(".sram.text")))
vmac_tx(void *pkt_handle, struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	uint16_t i; /* tbd index */
	volatile struct vmac_bd *tbdp; /* Tx BD pointer */
	int len;
	struct sk_buff *skb;
	uint32_t baddr;
	qdpc_pcie_bda_t *bda = vmp->bda;

	/* TODO: Under current architect, register_netdev() is called
	before EP is ready. So an variable ep_ready is added to achieve
	defensive programming. We need to change the code segment later */
	if (unlikely(vmp->ep_ready == 0)) {
#if defined(GPL_CODE)
		spin_unlock(&vmp->aei_tx_lock);	
#endif			
		vmac_tx_drop(pkt_handle, ndev);
#if defined(GPL_CODE)
		spin_lock(&vmp->aei_tx_lock);				
#endif
		return NETDEV_TX_OK;
	}
#if defined(GPL_CODE_TOOLBOX)
	if (vmp->usMirrorOutFlags != 0/* &&
	    __constant_ntohs(skb->protocol) != ETHER_TYPE_BRCM*/)
	{
		AEI_MultiMirrorPacket(pkt_handle, vmp->usMirrorOutFlags, FALSE);
	}
#endif
	vmp->vmac_xmit_cnt++;
#ifdef RC_TXDONE_TIMER
	spin_lock(&vmp->tx_lock);
#endif
	/* Tear down the previous skb transmitted by DMA */
	vmac_tx_teardown(ndev, bda);

	/* Reserve one entry space to differentiate full and empty case */
	if (vmp->vmac_tx_queue_len >= vmp->tx_bd_num - 2) {
#ifdef QTN_WAKEQ_SUPPORT
		vmac_try_stop_queue(ndev);
#endif
		if (vmp->vmac_tx_queue_len >= vmp->tx_bd_num - 1) {
#ifdef RC_TXDONE_TIMER
			spin_unlock(&vmp->tx_lock);
#endif
			vmp->tx_bd_busy_cnt++;
#if !defined(GPL_CODE)
			printk(KERN_ERR "%s fail to get BD\n", ndev->name);
#endif
#if defined(GPL_CODE)
		/* must free skb in case of memory leak in high throughput */
		skb_queue_head(&vmp->aei_teardown_queue, (struct sk_buff *)pkt_handle);
		return NETDEV_TX_OK;
#else
			return NETDEV_TX_BUSY;
#endif
		}
	}

	i = vmp->tx_bd_index;

	skb = (struct sk_buff *)pkt_handle;
#if defined(GPL_CODE)
	/*
	 * Hack TCP ACK:
	 * TCP ACK(without TCP option) is 54 bytes:
	 * 	Eth header(14 bytes) + IP header(20 bytes) + TCP header(20 bytes)
	 * Ethernet frame minimum length is 60 bytes.
	 * Linux network statck will add trailing bytes if length is less than 60.
	 *
	 * But if packet came from NSS fast path, then it doesn't add trailing bytes.
	 * It seems Quantenna wifi driver will drop packets less than 60 bytes.
	 * We hack the length to 60, don't worry about it because skb data buffer is
	 * always far more large than 60.
	 */
	if (unlikely(skb->len < 60))
		skb->len = 60;
	vmp->tx_skb[i] = skb;
#else
	vmp->tx_skb[i] = (struct sk_buff *)pkt_handle;
#endif
#ifdef QTN_SKB_RECYCLE_SUPPORT
	baddr = (uint32_t)pci_map_single(vmp->pdev, skb->data, skb->len, (int)DMA_BIDIRECTIONAL);
#else
	baddr = (uint32_t)pci_map_single(vmp->pdev, skb->data, skb->len, (int)DMA_TO_DEVICE);
#endif
	len = skb->len;
	wmb();

	/* Update local descriptor array */
	tbdp = &vmp->tx_bd_base[i];
	tbdp->buff_addr = baddr;

	/* Update remote Request Queue */
	VMAC_REG_WRITE(&bda->request[i].addr, (baddr));
	VMAC_REG_WRITE(&bda->request[i].info, (len | PCIE_TX_VALID_PKT));

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
    //zli add debug
    vmp->rqinfolist[vmp->infowindex].rqindex     = i;
    vmp->rqinfolist[vmp->infowindex].rqaddr      = baddr;
    vmp->rqinfolist[vmp->infowindex].rqvalue     = len | PCIE_TX_VALID_PKT;
    vmp->rqinfolist[vmp->infowindex].rqaddraddr  = (uint32_t)(&bda->request[i].addr);
    vmp->rqinfolist[vmp->infowindex].rqinfoaddr  = (uint32_t)(&bda->request[i].info);   
    if(++(vmp->infowindex)>= RECORD_LENGTH_ZLI)
        vmp->infowindex = 0;
#endif

	vmp->vmac_tx_queue_len++;

	dump_tx_pkt(vmp, bus_to_virt(baddr), len);

	if (++i >= vmp->tx_bd_num)
		i = 0;

	vmp->tx_bd_index = i;

	dump_tx_bd(vmp);

	writel(TOPAZ_SET_INT(IPC_EP_RX_PKT), vmp->ep_ipc_reg);

#ifdef RC_TXDONE_TIMER
	vmac_tx_teardown(ndev, bda);
	mod_timer(&vmp->tx_timer, jiffies + 1);
	spin_unlock(&vmp->tx_lock);
#endif
	return NETDEV_TX_OK;
}

static irqreturn_t vmac_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct vmac_priv *vmp = netdev_priv(ndev);

	handle_ep_rst_int(ndev);

	if(!vmp->msi_enabled) {
		/* Deassert remote INTx message */
		qdpc_deassert_intx(vmp);
	}

	vmp->intr_cnt++;

	vmac_napi_schedule(vmp);
#ifdef QTN_WAKEQ_SUPPORT
	vmac_try_wake_queue(ndev);
#endif
	dump_rx_int(vmp);

	return IRQ_HANDLED;
}

/*
 * The Tx ring has been full longer than the watchdog timeout
 * value. The transmitter must be hung?
 */
inline static void vmac_tx_timeout(struct net_device *ndev)
{
	printk(KERN_ERR "%s: vmac_tx_timeout: ndev=%p\n", ndev->name, ndev);
	ndev->trans_start = jiffies;
}

#ifdef RC_TXDONE_TIMER
static void vmac_tx_buff_cleaner(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	qdpc_pcie_bda_t *bda = vmp->bda;

	spin_lock(&vmp->tx_lock);
	vmac_tx_teardown(ndev, bda);

	if (vmp->tx_skb[vmp->tx_reclaim_start] == NULL) {
		del_timer(&vmp->tx_timer);
	} else {
		writel(TOPAZ_SET_INT(IPC_EP_RX_PKT), vmp->ep_ipc_reg);
		mod_timer(&vmp->tx_timer, jiffies + 1);
	}
	spin_unlock(&vmp->tx_lock);
}
#endif

/* ethtools support */
static int vmac_get_settings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
	return -EINVAL;
}

static int vmac_set_settings(struct net_device *ndev, struct ethtool_cmd *cmd)
{

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	return -EINVAL;
}

static int vmac_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd)
{
	int ret = 0;
	struct vmac_priv *vmp = NULL;
#if defined(GPL_CODE_TOOLBOX)
	MirrorCfg mirrorCfg;
#endif

	if (!ndev)
		return -ENETDOWN;

	vmp = netdev_priv(ndev);

	switch (cmd) {
		case SIOCDEVPRIVATE :
			break;
#if defined(GPL_CODE_TOOLBOX)
		case SIOCPORTMIRROR:
		{
			if (copy_from_user((void *)&mirrorCfg, (int *)rq->ifr_data, sizeof(MirrorCfg)))
				return -EFAULT;
			else
			{
				if ( mirrorCfg.nDirection == MIRROR_DIR_IN )
				{
					AEI_UpdateMirrorFlag(&(vmp->usMirrorInFlags),
						  mirrorCfg.szMirrorInterface, mirrorCfg.nStatus);
				}
				else /* MIRROR_DIR_OUT */
				{
					AEI_UpdateMirrorFlag(&(vmp->usMirrorOutFlags),
						mirrorCfg.szMirrorInterface, mirrorCfg.nStatus);
				}
				ndev->is_use_mcast_macaddr = TRUE;
			}
			ret = 0;
			goto out;
		}
#endif /* AEI_VDSL_TOOLBOX */
		default:
			ret = -EINVAL;
			goto out;
	}

out:
	return ret;
}

static void vmac_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *info)
{
	struct vmac_priv *vmp = netdev_priv(ndev);

	strcpy(info->driver, DRV_NAME);
	strcpy(info->version, DRV_VERSION);
	info->fw_version[0] = '\0';
	sprintf(info->bus_info, "%s %d", DRV_NAME, vmp->mac_id);
	info->regdump_len = 0;
}

static const struct ethtool_ops vmac_ethtool_ops = {
	.get_settings = vmac_get_settings,
	.set_settings = vmac_set_settings,
	.get_drvinfo = vmac_get_drvinfo,
	.get_link = ethtool_op_get_link,
};

static const struct net_device_ops vmac_device_ops = {
	.ndo_open = vmac_open,
	.ndo_stop = vmac_close,
	.ndo_start_xmit = vmac_xmit,
	.ndo_do_ioctl = vmac_ioctl,
	.ndo_tx_timeout = vmac_tx_timeout,
	.ndo_set_mac_address = eth_mac_addr,
};

struct net_device *vmac_alloc_ndev(void)
{
	struct net_device * ndev;

        /* Allocate device structure */
	ndev = alloc_netdev(sizeof(struct vmac_priv), vmaccfg.ifname, ether_setup);
	if(!ndev)
		printk(KERN_ERR "%s: alloc_etherdev failed\n", vmaccfg.ifname);
#if defined(GPL_CODE)
        /*
         * Set PHY type
         * See BCM's wl_alloc_linux_if()
         */
        netdev_path_set_hw_port(ndev, 0, BLOG_EXTRA1PHY);
#endif

	return ndev;
}
EXPORT_SYMBOL(vmac_alloc_ndev);

static void eth_parse_enetaddr(const char *addr, uint8_t *enetaddr)
{
	char *end;
	int i;

	for (i = 0; i < 6; ++i) {
		enetaddr[i] = addr ? simple_strtoul(addr, &end, 16) : 0;
		if (addr)
			addr = (*end) ? end + 1 : end;
	}
}


int vmac_net_init(struct pci_dev *pdev)
{
	struct vmac_priv *vmp = NULL;
	struct net_device *ndev = NULL;
	int err = -ENOMEM;
	__iomem qdpc_pcie_bda_t *bda;

	printk(KERN_INFO"%s version %s %s\n", DRV_NAME, DRV_VERSION, DRV_AUTHOR);

	ndev = (struct net_device *)pci_get_drvdata(pdev);
	if (!ndev)
		goto vnet_init_err_0;


	if (ethaddr)
		eth_parse_enetaddr(ethaddr, ndev->dev_addr);

	if (!is_valid_ether_addr(ndev->dev_addr))
		random_ether_addr(ndev->dev_addr);

	ndev->netdev_ops = &vmac_device_ops;
	ndev->tx_queue_len = QTN_GLOBAL_INIT_EMAC_TX_QUEUE_LEN;
	SET_ETHTOOL_OPS(ndev, &vmac_ethtool_ops);

	/* Initialize private data */
	vmp = netdev_priv(ndev);
	vmp->pdev = pdev;
	vmp->ndev = ndev;

	vmp->pcfg = &vmaccfg;
	vmp->tx_bd_num = vmp->pcfg->tx_bd_num;
	vmp->rx_bd_num = vmp->pcfg->rx_bd_num;

#ifdef QTN_SKB_RECYCLE_SUPPORT
	spin_lock_init(&vmp->rx_skb_freelist_lock);
	skb_queue_head_init(&vmp->rx_skb_freelist);
	vmp->rx_skb_freelist_fill_level = QTN_RX_SKB_FREELIST_FILL_SIZE;
	vmp->skb_recycle_cnt = 0;
	vmp->skb_recycle_failures = 0;
#endif

	if (vmp->tx_bd_num > PCIE_RC_TX_QUEUE_LEN) {
		printk("Error: The length of TX BD array should be no more than %d\n",
				PCIE_RC_TX_QUEUE_LEN);
		goto vnet_init_err_0;
	}

	vmp->ep_ipc_reg = (unsigned long)
		QDPC_BAR_VADDR(vmp->sysctl_bar, TOPAZ_IPC_OFFSET);
	ndev->irq = pdev->irq;

	ndev->if_port = QDPC_PLATFORM_IFPORT;

	ndev->watchdog_timeo = VMAC_TX_TIMEOUT;

	bda = vmp->bda;

	qdpc_pcie_posted_write(vmp->tx_bd_num, &bda->bda_rc_tx_bd_num);
	qdpc_pcie_posted_write(vmp->rx_bd_num, &bda->bda_rc_rx_bd_num);

	/* Allocate Tx & Rx SKB descriptor array */
	if (alloc_skb_desc_array(ndev))
		goto vnet_init_err_0;

	/* Allocate and initialise Tx & Rx descriptor array */
	if (alloc_bd_tbl(ndev))
		goto vnet_init_err_1;

#ifdef QTN_SKB_RECYCLE_SUPPORT
	__vmac_rx_skb_freelist_refill(vmp);
#endif

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)
    vmp->rqinfolist = (struct rqinfo*)kzalloc(RECORD_LENGTH_ZLI * sizeof(struct rqinfo), GFP_KERNEL);   
    if (!vmp->rqinfolist){
        printk("zli debug info alloc failed\n");
    }
    vmp->infowindex = 0;
#endif
	if (alloc_and_init_rxbuffers(ndev))
		goto vnet_init_err_2;

	/* Initialize NAPI */
#if defined(GPL_CODE)
	netif_napi_add(ndev, &vmp->napi, vmac_rx_poll, 64);
#else
	netif_napi_add(ndev, &vmp->napi, vmac_rx_poll, 10);
#endif
	/* Register device */
	if ((err = register_netdev(ndev)) != 0) {
		printk(KERN_ERR "%s: Cannot register net device, error %d\n", DRV_NAME, err);
		goto vnet_init_err_3;
	}
	printk(KERN_INFO"%s: Vmac Ethernet found\n", ndev->name);

	/* Add the device attributes */
	err = sysfs_create_group(&ndev->dev.kobj, &vmac_attr_group);
	if (err) {
		printk(KERN_ERR "Error creating sysfs files\n");
	}

	enable_ep_rst_detection(ndev);

	vmp->show_item = SHOW_VMAC_STATS;

#if defined(GPL_CODE)
	spin_lock_init(&vmp->aei_tx_lock);
	skb_queue_head_init(&vmp->aei_teardown_queue);
#endif

#ifdef RC_TXDONE_TIMER
	spin_lock_init(&vmp->tx_lock);
	init_timer(&vmp->tx_timer);
	vmp->tx_timer.data = (unsigned long)ndev;
	vmp->tx_timer.function = (void (*)(unsigned long))&vmac_tx_buff_cleaner;
#endif
	spin_lock_init(&vmp->txqueue_op_lock);

#ifdef QTN_TX_SKBQ_SUPPORT
	vmp->tx_skbq_budget = QTN_RC_TX_BUDGET;
	/* Make the max size of skbq as four times of tx descriptor ring size */
	vmp->tx_skbq_max_size = vmp->tx_bd_num << 4;
	vmp->tx_skbq_tasklet_budget = QTN_RC_TX_TASKLET_BUDGET;
	spin_lock_init(&vmp->tx_skbq_lock);
	skb_queue_head_init(&vmp->tx_skb_queue);
	tasklet_init(&vmp->tx_skbq_tasklet, vmac_tx_skbq_tasklet, (unsigned long)ndev);
#endif

#ifdef QTN_SKB_RECYCLE_SUPPORT
	__vmac_rx_skb_freelist_refill(vmp);
#endif

	return 0;

vnet_init_err_3:
	free_rx_skbs(vmp);
vnet_init_err_2:
#ifdef QTN_SKB_RECYCLE_SUPPORT
	vmac_rx_skb_freelist_purge(vmp);
#endif
	free_bd_tbl(vmp);
vnet_init_err_1:
	free_skb_desc_array(ndev);
vnet_init_err_0:
	return err;
}
EXPORT_SYMBOL(vmac_net_init);

static void free_rx_skbs(struct vmac_priv *vmp)
{
	/* All Ethernet activity should have ceased before calling
	 * this function
	 */
	uint16_t i;
	for (i = 0; i < vmp->rx_bd_num; i++) {
		if (vmp->rx_skb[i]) {
			dev_kfree_skb(vmp->rx_skb[i]);
			vmp->rx_skb[i] = 0;
		}
	}
}

static void free_tx_skbs(struct vmac_priv *vmp)
{
	/* All Ethernet activity should have ceased before calling
	 * this function
	 */
	uint16_t i;
	for (i = 0; i < vmp->tx_bd_num; i++) {
		if (vmp->tx_skb[i]) {
			dev_kfree_skb(vmp->tx_skb[i]);
			vmp->tx_skb[i] = 0;
		}
	}
}

static void init_tx_bd(struct vmac_priv *vmp)
{
	uint16_t i;
	for (i = 0; i< vmp->tx_bd_num; i++)
		vmp->tx_bd_base[i].buff_info |= cpu_to_le32(VMAC_BD_EMPTY);
}

static int alloc_and_init_rxbuffers(struct net_device *ndev)
{
	uint16_t i;
	struct vmac_priv *vmp = netdev_priv(ndev);

	memset((void *)vmp->rx_bd_base, 0, vmp->rx_bd_num * VMAC_BD_LEN);

	/* Allocate rx buffers */
	for (i = 0; i < vmp->rx_bd_num; i++) {
		if (skb2rbd_attach(ndev, i, 0)) {
			return -1;
		}
	}

	vmp->rx_bd_base[vmp->rx_bd_num - 1].buff_info |= cpu_to_le32(VMAC_BD_WRAP);
	return 0;
}

extern int qdpc_unmap_iomem(struct vmac_priv *priv);
void vmac_clean(struct net_device *ndev)
{
	struct vmac_priv *vmp;

	if (!ndev)
		return;

	vmp = netdev_priv(ndev);
#ifdef GPL_CODE_QTN_RELOAD
    /* device_remove_file(&ndev->dev, &dev_attr_dbg);
     * we should remove all device attributes
    */
    sysfs_remove_group(&ndev->dev.kobj, &vmac_attr_group);
#endif
	unregister_netdev(ndev);

#if defined(GPL_CODE_TX_HANGUP_WORKAROUND)    
    kfree(vmp->rqinfolist);
#endif    
	free_rx_skbs(vmp);
	free_tx_skbs(vmp);
	free_skb_desc_array(ndev);
#ifdef QTN_SKB_RECYCLE_SUPPORT
	vmac_rx_skb_freelist_purge(vmp);
#endif

	disable_ep_rst_detection(ndev);

	netif_napi_del(&vmp->napi);

	free_bd_tbl(vmp);
}

static void bring_up_interface(struct net_device *ndev)
{
	/* Interface will be ready to send/receive data, but will need hooking
	 * up to the interrupts before anything will happen.
	 */
	struct vmac_priv *vmp = netdev_priv(ndev);
	enable_vmac_ints(vmp);
}

static void shut_down_interface(struct net_device *ndev)
{
	struct vmac_priv *vmp = netdev_priv(ndev);
	/* Close down MAC and DMA activity and clear all data. */
	disable_vmac_ints(vmp);
}


static int vmac_open(struct net_device *ndev)
{
	int retval = 0;
	struct vmac_priv *vmp = netdev_priv(ndev);

	bring_up_interface(ndev);

	napi_enable(&vmp->napi);

	/* Todo: request_irq here */
	retval = request_irq(ndev->irq, &vmac_interrupt, 0, ndev->name, ndev);
	if (retval) {
		printk(KERN_ERR "%s: unable to get IRQ %d\n",
			ndev->name, ndev->irq);
		goto err_out;
	}

	netif_start_queue(ndev);

	vmac_irq_open_fixup(vmp);

	return 0;
err_out:
	napi_disable(&vmp->napi);
	return retval;
}

static int vmac_close(struct net_device *ndev)
{
	struct vmac_priv *const vmp = netdev_priv(ndev);

	napi_disable(&vmp->napi);

	shut_down_interface(ndev);

	netif_stop_queue(ndev);

	free_irq(ndev->irq, ndev);

	return 0;
}

