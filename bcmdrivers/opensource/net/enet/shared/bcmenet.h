/*
 Copyright 2002-2010 Broadcom Corp. All Rights Reserved.

 <:label-BRCM:2011:DUAL/GPL:standard    
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2, as published by
 the Free Software Foundation (the "GPL").
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 
 A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
 writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 
 :>
*/
#ifndef _BCMENET_H_
#define _BCMENET_H_

#ifndef FAP_4KE
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/nbuff.h>
#include "boardparms.h"
#include "bcm_OS_Deps.h"
#include <linux/bcm_log.h>
#include <bcmnet.h>
#include <bcm/bcmswapitypes.h>
#include <linux/version.h>
#endif

#if defined(PKTC)
#include <linux_osl_dslcpe_pktc.h>
#endif

#include "bcmtypes.h"
#include "bcmenet_common.h"
/* Macros need to be defined after hardware dependent headers */
#define NUM_PORTS                   1

#if defined(GPL_CODE)
#define WHOLE_N_MAX_THRESHOLD 3
#define AEI_IS_CPU_FREE() (((avenrun[0] + (FIXED_1/200)) >> FSHIFT) < WHOLE_N_MAX_THRESHOLD ?1 : 0)
#endif


#if defined(ENET_EPON_CONFIG)
#define MAX_EPON_IFS 8 //eight LLID
extern struct net_device* eponifid_to_dev[];
extern struct net_device* oam_dev;
extern void *oam_tx_func;
extern void *epon_data_tx_func;
#endif

#if defined(ENET_GPON_CONFIG)
extern struct net_device* gponifid_to_dev[];
#endif

#ifdef RDPA_VPORTS
extern struct net_device *rdpa_vport_to_dev[];
#endif

#if defined(_BCMENET_LOCAL_)
#define ENET_POLL_DONE        0x80000000

typedef struct {
    unsigned int extPhyMask;
    int dump_enable;
    struct net_device_stats net_device_stats_from_hw;
    BcmEnet_devctrl *pVnetDev0_g;
    /* The following fields are for impl6 */
    struct task_struct *rx_thread;
    wait_queue_head_t rx_thread_wqh;
    int rx_work_avail;
    spinlock_t ethlock_rx;
    spinlock_t ethlock_tx;
}enet_global_var_t;

extern enet_global_var_t global;
extern struct net_device *vnet_dev[];
extern int vport_to_logicalport[];
extern int logicalport_to_vport[];
#define EnetGetEthernetMacInfo() (((BcmEnet_devctrl *)netdev_priv(vnet_dev[0]))->EnetInfo)

#define LOGICAL_PORT_TO_VPORT(p) logicalport_to_vport[(p)]
#define VPORT_TO_LOGICAL_PORT(p) vport_to_logicalport[(p)]
#define CBPORT_TO_VPORTIDX(p) cbport_to_vportIdx[p]
inline int IsLogPortWan( int log_port );

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
#define BCM_SW_MAX_TRUNK_GROUPS           (2)
#define TRUNK_GRP_PORT_MASK               (0x0000FFFF) /* lower 16 bits */
#define TRUNK_GRP_MASTER_PORT_MASK        (0xFFFF0000) /* upper 16 bits */
#define TRUNK_GRP_MASTER_PORT_SHIFT       (16) /* upper 16 bits */
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

#ifndef FAP_4KE

#ifdef DYING_GASP_API


/* Flag indicates we're in Dying Gasp and powering down - don't clear once set */
extern int dg_in_context; 

#define ENET_TX_LOCK() if(dg_in_context==0) spin_lock_bh(&global.pVnetDev0_g->ethlock_tx)
#define ENET_TX_UNLOCK() if(dg_in_context==0) spin_unlock_bh(&global.pVnetDev0_g->ethlock_tx)
#define ENET_RX_LOCK() if(dg_in_context==0) spin_lock_bh(&global.pVnetDev0_g->ethlock_rx)
#define ENET_RX_UNLOCK() if(dg_in_context==0) spin_unlock_bh(&global.pVnetDev0_g->ethlock_rx)
#define ENET_MOCA_TX_LOCK() if(dg_in_context==0) spin_lock_bh(&global.pVnetDev0_g->ethlock_moca_tx)
#define ENET_MOCA_TX_UNLOCK() if(dg_in_context==0) spin_unlock_bh(&global.pVnetDev0_g->ethlock_moca_tx)

#else


#define ENET_TX_LOCK() spin_lock_bh(&global.pVnetDev0_g->ethlock_tx)
#define ENET_TX_UNLOCK() spin_unlock_bh(&global.pVnetDev0_g->ethlock_tx)
#define ENET_RX_LOCK() spin_lock_bh(&global.pVnetDev0_g->ethlock_rx)
#define ENET_RX_UNLOCK() spin_unlock_bh(&global.pVnetDev0_g->ethlock_rx)
#define ENET_MOCA_TX_LOCK() spin_lock_bh(&global.pVnetDev0_g->ethlock_moca_tx)
#define ENET_MOCA_TX_UNLOCK() spin_unlock_bh(&global.pVnetDev0_g->ethlock_moca_tx)

#ifdef GPL_CODE_ABBA_FIX
//we know the printk below could cause problem, but for make sure we experienced this problem,
//still leave there;
//NOTE: the production code should remove following printk.
#define MAX_LOCKUP_DETECT 10000
#define ENET_RX_LOCK_WITH_UNLOCK_TX() \
{ \
	unsigned int loop_counter=0; \
	while(!spin_trylock_bh(&global.pVnetDev0_g->ethlock_rx)){ \
		if(!spin_trylock_bh(&global.pVnetDev0_g->ethlock_tx)){ \
			if(loop_counter++>MAX_LOCKUP_DETECT){ \
				spin_unlock_bh(&global.pVnetDev0_g->ethlock_tx); \
				spin_lock_bh(&global.pVnetDev0_g->ethlock_tx); \
			} \
		} else {\
			spin_unlock_bh(&global.pVnetDev0_g->ethlock_tx); \
		} \
	} \
}
#endif

#endif /* ELSE #ifdef DYING_GASP_API */


#endif
#endif /* #if defined(_BCMENET_LOCAL_) */

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 5
#define local_unreachable() __builtin_unreachable()
#else
#define local_unreachable() do { } while(0)
#endif

#define BULK_RX_LOCK_ACTIVE() pDevCtrl->bulk_rx_lock_active[cpuid]
#define RECORD_BULK_RX_LOCK() pDevCtrl->bulk_rx_lock_active[cpuid] = 1
#define RECORD_BULK_RX_UNLOCK() pDevCtrl->bulk_rx_lock_active[cpuid] = 0
#define DMA_THROUGHPUT_TEST_EN  0x80000

//#if !defined(RDPA_PLATFORM)
#if !defined(CONFIG_BCM96838) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) && !defined(CONFIG_BCM96848)
#include "bcmenet_dma.h"
#else
#include "bcmenet_runner.h"
#endif

#if defined(_BCMENET_LOCAL_)
int bcmeapi_create_vport(struct net_device *dev);
void bcmeapi_napi_post(struct BcmEnet_devctrl *pDevCtrl);
void ethsw_phyport_rreg2(int phy_id, int reg, uint16 *data, int flags);
void ethsw_phyport_wreg2(int phy_id, int reg, uint16 *data, int flags);
PHY_STAT enet_get_ext_phy_stat(int unit, int phy_port, int cb_port);
/*
 * IMPORTANT: The following 3 macros are only used for ISR context. The
 * recycling context is defined by enet_recycle_context_t
 */
#define CONTEXT_CHAN_MASK   0x3
#define BUILD_CONTEXT(pDevCtrl,channel) \
            (uint32)((uint32)(pDevCtrl) | ((uint32)(channel) & CONTEXT_CHAN_MASK))
#define CONTEXT_TO_PDEVCTRL(context)    (BcmEnet_devctrl*)((context) & ~CONTEXT_CHAN_MASK)
#define CONTEXT_TO_CHANNEL(context)     (int)((context) & CONTEXT_CHAN_MASK)

unsigned short bcm_type_trans(struct sk_buff *skb, struct net_device *dev);
#endif /* _BCMENET_LOCAL_ */

int bcm63xx_enet_getPortFromName(char *pIfName, int *pUnit, int *pPort);
int bcm63xx_enet_getPortmapFromName(char *pIfName, int *pUnit, unsigned int *pPortmap);

/* int bcmeapi_map_interrupt(BcmEnet_devctrl *pDevCtrl) */
#define BCMEAPI_INT_MAPPED_INTPHY (1<<0)
#define BCMEAPI_INT_MAPPED_EXTPHY (1<<1)

#if defined(RXCHANNEL_PKT_RATE_LIMIT)
#endif /* defined(RXCHANNEL_PKT_RATE_LIMIT) */

#ifdef DYING_GASP_API
int enet_send_dying_gasp_pkt(void);
#endif /* DYING_GASP_API */

#if defined(GPL_CODE_SW_WAN_ETH_LED) || defined(GPL_CODE_INET_LED_BLINK)
void AEI_poll_gphy_LED(void);
#endif
void bcmeapi_reset_mib_cnt(uint32_t sw_port);
struct net_device *enet_phyport_to_vport_dev(int port);
struct net_device *phyPortId_to_netdev(int logical_port, int gemid);

int bcm63xx_enet_isExtSwPresent(void);
int bcm63xx_enet_intSwPortToExtSw(void);
unsigned int bcm63xx_enet_extSwId(void);
void bcmeapi_enet_module_cleanup(void);
uint32 ConfigureJumboPort(uint32 regVal, int portVal, unsigned int configVal);
void bcmeapi_module_init2(void);
void link_change_handler(int port, int cb_port, int linkstatus, int speed, int duplex);
int enet_get_next_crossbar_port(int logPort, int cb_port);
#define enet_get_first_crossbar_port(logPort) enet_get_next_crossbar_port(logPort, BP_CROSSBAR_NOT_DEFINED)

#endif /* _BCMENET_H_ */

