/*SH0
*******************************************************************************
**                                                                           **
**         Copyright (c) 2011-2012 Quantenna Communications, Inc             **
**                            All Rights Reserved                            **
**                                                                           **
*******************************************************************************
**                                                                           **
**  Redistribution and use in source and binary forms, with or without       **
**  modification, are permitted provided that the following conditions       **
**  are met:                                                                 **
**  1. Redistributions of source code must retain the above copyright        **
**     notice, this list of conditions and the following disclaimer.         **
**  2. Redistributions in binary form must reproduce the above copyright     **
**     notice, this list of conditions and the following disclaimer in the   **
**     documentation and/or other materials provided with the distribution.  **
**  3. The name of the author may not be used to endorse or promote products **
**     derived from this software without specific prior written permission. **
**                                                                           **
**  Alternatively, this software may be distributed under the terms of the   **
**  GNU General Public License ("GPL") version 2, or (at your option) any    **
**  later version as published by the Free Software Foundation.              **
**                                                                           **
**  In the case this software is distributed under the GPL license,          **
**  you should have received a copy of the GNU General Public License        **
**  along with this software; if not, write to the Free Software             **
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  **
**                                                                           **
**  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR       **
**  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES**
**  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  **
**  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,         **
**  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT **
**  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,**
**  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    **
**  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      **
**  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF **
**  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.        **
**                                                                           **
*******************************************************************************
EH0*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/io.h>

#include <linux/timer.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <qtn/dmautil.h>
#include <drivers/ruby/dma_cache_ops.h>
#include <asm/board/board_config.h>
#include "net80211/ieee80211.h"

#include <qtn/topaz_tqe.h>
#include <qtn/topaz_hbm_cpuif.h>
#include <qtn/topaz_hbm.h>
#include "topaz_pcie_tqe.h"
#include "topaz_vnet.h"


struct tqe_netdev_priv {
	struct napi_struct napi;
	struct net_device_stats stats;
	struct net_device *pcie_ndev;

	ALIGNED_DMA_DESC(union, topaz_tqe_pcieif_descr) rx;
};

static void tqe_irq_enable(void)
{
#ifdef PCIE_TQE_INTR_WORKAROUND
	__topaz_tqe_cpuif_setup_irq(TOPAZ_TQE_PCIE_REL_PORT, 1, 0);
#else
	uint32_t temp;
        temp = readl(TOPAZ_LH_IPC3_INT_MASK);
        temp |= 0x1;
        writel(temp, TOPAZ_LH_IPC3_INT_MASK);
#endif
}

static void tqe_irq_disable(void)
{
#ifndef PCIE_TQE_INTR_WORKAROUND
	uint32_t temp;
        temp = readl(TOPAZ_LH_IPC3_INT_MASK);
        temp &= ~0x1;
        writel(temp, TOPAZ_LH_IPC3_INT_MASK);
#endif
}

static union topaz_tqe_pcieif_descr * desc_bus_to_uncached(struct tqe_netdev_priv *priv, void *_bus_desc)
{
	unsigned long bus_desc = (unsigned long)_bus_desc;
	unsigned long bus_start = priv->rx.descs_dma_addr;
	unsigned long virt_start = (unsigned long)&priv->rx.descs[0];
	return (void *)(bus_desc - bus_start + virt_start);
}

static int __sram_text tqe_rx_napi_handler(struct napi_struct *napi, int budget)
{
	int processed = 0;
	struct tqe_netdev_priv *priv = container_of(napi, struct tqe_netdev_priv, napi);

	topaz_hbm_filter_txdone_pool();

	while (processed < budget) {
		union topaz_tqe_cpuif_status status;
		union topaz_tqe_pcieif_descr __iomem *bus_desc;
		union topaz_tqe_pcieif_descr *uncached_virt_desc;

		status = topaz_tqe_pcieif_get_status();
		if (status.data.empty) {
			break;
		}

		bus_desc = topaz_tqe_pcieif_get_curr();
		uncached_virt_desc = desc_bus_to_uncached(priv, bus_desc);

		if (likely(uncached_virt_desc->data.own)) {
			if (vmac_tx(uncached_virt_desc, priv->pcie_ndev, PKT_TQE)
				== NETDEV_TX_OK)
				topaz_tqe_pcieif_put_back(bus_desc);

			++processed;
		} else {
			printk("%s unowned descriptor? bus_desc 0x%p\n",
				__FUNCTION__, bus_desc);
			break;
		}
	}

	if (processed < budget) {
		napi_complete(napi);
		tqe_irq_enable();
	}

	return processed;
}

static irqreturn_t __sram_text tqe_irqh(int irq, void *_dev)
{
	struct net_device *dev = _dev;
	struct tqe_netdev_priv *priv = netdev_priv(dev);
	uint32_t ipcstat;

	ipcstat = readl(TOPAZ_LH_IPC3_INT) & 0xffff;
	if (ipcstat) {
		writel(ipcstat << 16, TOPAZ_LH_IPC3_INT);

		if(ipcstat & 0x1) {
			napi_schedule(&priv->napi);
			tqe_irq_disable();
		}
	}

	return IRQ_HANDLED;
}

/*
 * TQE network device ops
 */
static int tqe_ndo_open(struct net_device *dev)
{
	return -ENODEV;
}

static int tqe_ndo_stop(struct net_device *dev)
{
	return -ENODEV;
}

extern int fwt_if_get_index_from_mac_be(const uint8_t *mac_be);
fwt_db_entry *vmac_get_tqe_ent(const unsigned char *src_mac_be, const unsigned char *dst_mac_be)
{
	int index = 0;
	fwt_db_entry *fwt_ent, *fwt_ent_out;

	index = fwt_if_get_index_from_mac_be(dst_mac_be);
	if (index < 0) {
		return NULL;
	}
	fwt_ent = fwt_db_get_table_entry(index);
	if (fwt_ent && fwt_ent->valid) {
		fwt_ent_out = fwt_ent;
	} else {
		return NULL;
	}

	index = fwt_if_get_index_from_mac_be(src_mac_be);
	if (index < 0) {
		return NULL;
	}
	fwt_ent = fwt_db_get_table_entry(index);
	if (!fwt_ent || !fwt_ent->valid)
		return NULL;

	return fwt_ent_out;
}

int topaz_pcie_tqe_xmit(fwt_db_entry *fwt_ent, void *data_bus, int data_len)
{
	union topaz_tqe_cpuif_ppctl ctl;
	uint8_t port;
	uint8_t node;
	uint8_t tid = WME_AC_TO_TID(0);/* TODO: */
	uint16_t misc_user;
	int8_t pool = topaz_hbm_payload_get_pool_bus(data_bus);
	const long buff_ptr_offset =
		topaz_hbm_payload_buff_ptr_offset_bus(data_bus, pool, NULL);

	port = fwt_ent->out_port;
	node = fwt_ent->out_node;
	misc_user = 0;

	topaz_tqe_cpuif_ppctl_init(&ctl,
			port, &node, 1, tid,
			1, 1, 0, 1, misc_user);

	ctl.data.pkt = (void *)data_bus;
	ctl.data.buff_ptr_offset = buff_ptr_offset;
	ctl.data.length = data_len;
	ctl.data.buff_pool_num = pool;

	while (topaz_tqe_pcieif_tx_nready());

	topaz_tqe_pcieif_tx_start(&ctl);

	return NET_XMIT_SUCCESS;
}

static int tqe_ndo_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	return NETDEV_TX_BUSY;
}

static const struct net_device_ops tqe_ndo = {
	.ndo_open = tqe_ndo_open,
	.ndo_stop = tqe_ndo_stop,
	.ndo_start_xmit = tqe_ndo_start_xmit,
	.ndo_set_mac_address = eth_mac_addr,
};

static int tqe_descs_alloc(struct tqe_netdev_priv *priv)
{
	int i;
	union topaz_tqe_pcieif_descr __iomem *bus_descs;

	if (ALIGNED_DMA_DESC_ALLOC(&priv->rx, QTN_BUFS_PCIE_TQE_RX_RING, TOPAZ_TQE_CPUIF_RXDESC_ALIGN, 0)) {
		return -ENOMEM;
	}

	bus_descs = (void *)priv->rx.descs_dma_addr;
	for (i = 0; i < QTN_BUFS_PCIE_TQE_RX_RING; i++) {
		priv->rx.descs[i].data.next = &bus_descs[(i + 1) % QTN_BUFS_PCIE_TQE_RX_RING];
	}

	printk(KERN_INFO "%s: %u tqe_rx_descriptors at kern uncached 0x%p bus 0x%p\n",
			__FUNCTION__, priv->rx.desc_count, priv->rx.descs, bus_descs);

	topaz_tqe_pcieif_setup_ring((void *)priv->rx.descs_dma_addr, priv->rx.desc_count);

	return 0;
}

static void tqe_descs_free(struct tqe_netdev_priv *priv)
{
	if (priv->rx.descs) {
		ALIGNED_DMA_DESC_FREE(&priv->rx);
	}
}

struct net_device * tqe_pcie_netdev_init( struct net_device *pcie_ndev)
{
	int rc = 0;
	struct net_device *dev = NULL;
	struct tqe_netdev_priv *priv;
	static const int tqe_netdev_irq = 18;
#ifdef TOPAZ_PCIE_HDP_TX_QUEUE
	struct tqe_queue *tx_queue;
#endif

	dev = alloc_netdev(sizeof(struct tqe_netdev_priv), "tqe_pcie", &ether_setup);
	if (!dev) {
		printk(KERN_ERR "%s: unable to allocate dev\n", __FUNCTION__);
		goto netdev_alloc_error;
	}
	priv = netdev_priv(dev);

	dev->base_addr = 0;
	dev->irq = tqe_netdev_irq;
	dev->watchdog_timeo = 60 * HZ;
	dev->tx_queue_len = 1;
	dev->netdev_ops = &tqe_ndo;

	/* Initialise TQE */
	topaz_tqe_pcieif_setup_reset(1);
	topaz_tqe_pcieif_setup_reset(0);

	if (tqe_descs_alloc(priv)) {
		goto desc_alloc_error;
	}

	rc = request_irq(dev->irq, &tqe_irqh, 0, dev->name, dev);
	if (rc) {
		printk(KERN_ERR "%s: unable to get %s IRQ %d\n",
				__FUNCTION__, dev->name, tqe_netdev_irq);
		goto irq_request_error;
	}

	rc = register_netdev(dev);
	if (rc) {
		printk(KERN_ERR "%s: Cannot register net device '%s', error %d\n",
				__FUNCTION__, dev->name, rc);
		goto netdev_register_error;
	}

	priv->pcie_ndev = pcie_ndev;

#ifdef TOPAZ_PCIE_HDP_TX_QUEUE
	tx_queue = &priv->tqe_tx_queue;
	tx_queue->queue_in = 0;
	tx_queue->queue_out = 0;
	tx_queue->pkt_num = 0;
	init_timer(&tx_queue->tx_timer);
	tx_queue->tx_timer.data = (unsigned long)priv;
	tx_queue->tx_timer.function = (void (*)(unsigned long))&tqe_queue_start_tx;
#endif

	netif_napi_add(dev, &priv->napi, &tqe_rx_napi_handler, board_napi_budget());
	napi_enable(&priv->napi);
	tqe_irq_enable();
#ifdef PCIE_TQE_INTR_WORKAROUND
        writel(readl(TOPAZ_LH_IPC3_INT_MASK) | 0x1, TOPAZ_LH_IPC3_INT_MASK);
#endif
	return dev;

netdev_register_error:
	free_irq(dev->irq, dev);
irq_request_error:
	tqe_descs_free(priv);
desc_alloc_error:
	free_netdev(dev);
netdev_alloc_error:
	return NULL;
}
EXPORT_SYMBOL(tqe_pcie_netdev_init);

void tqe_netdev_exit(struct net_device *dev)
{
	struct tqe_netdev_priv *priv = netdev_priv(dev);
	tqe_descs_free(priv);
	free_netdev(dev);
}
EXPORT_SYMBOL(tqe_netdev_exit);

