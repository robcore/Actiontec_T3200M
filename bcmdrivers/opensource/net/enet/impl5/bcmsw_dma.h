/*
    Copyright 2004-2010 Broadcom Corporation

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

#ifndef _BCMSW_DMA_H_
#define _BCMSW_DMA_H_

#include <bcm/bcmswapitypes.h>
#include <bcm/bcmswapistat.h>

/****************************************************************************
    Prototypes
****************************************************************************/
int  ethsw_save_port_state(void);
int  ethsw_restore_port_state(void);
void bcmeapi_ethsw_dump_page(int page);
int  bcmeapi_ethsw_dump_mib(int port, int type);
int  bcmeapi_ioctl_ethsw_port_tagreplace(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagmangle(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagmangle_matchvid(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagstrip(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_pause_capability(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_prio_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_vlan(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_pbvlan(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_irc_set(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_irc_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_erc_set(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_erc_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_config(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_sched(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_port_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_rxchannel_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_txchannel_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_txq_sel(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_clear_port_stats(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_clear_stats(uint32_t port_map);
int  bcmeapi_ioctl_ethsw_counter_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_default_tag_config(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_traffic_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_loopback(struct ethswctl_data *e, int phy_id);
int  bcmeapi_ioctl_ethsw_phy_mode(struct ethswctl_data *e, int phy_id);
int  bcmeapi_ioctl_ethsw_pkt_padding(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_jumbo_control(struct ethswctl_data *e); // bill
void fast_age_all(uint8_t age_static);
int bcmeapi_ioctl_ethsw_dscp_to_priority_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_ethsw_pcp_to_priority_mapping(struct ethswctl_data *e);
void ethsw_set_wanoe_portmap(uint16 wan_port_map);
int bcmeapi_ethsw_init(void);
void bcmeapi_ethsw_init_ports(void);
void bcmeapi_ethsw_init_hw(int unit, uint32_t map, int wanPort);
void bcmeapi_set_multiport_address(uint8_t* addr);
int ethsw_set_mac_hw(uint16_t sw_port, PHY_STAT ps);
uint16_t ethsw_get_pbvlan(int port);
void ethsw_set_pbvlan(int port, uint16_t fwdMap);

int reset_switch(void);


#if (defined(CONFIG_BCM_ARL) || defined(CONFIG_BCM_ARL_MODULE))
int enet_hook_for_arl_access(void *ethswctl);
#endif

int bcmeapi_ioctl_debug_conf(struct ethswctl_data *e);
int write_vlan_table(bcm_vlan_t vid, uint32_t val32);
void enet_arl_write(uint8_t *mac, uint16_t vid, int val);
void bcmeapi_ethsw_set_stp_mode(unsigned int unit, unsigned int port, unsigned char stpState);
#ifdef REPORT_HARDWARE_STATS
int ethsw_get_hw_stats(int port, struct net_device_stats *stats);
#endif
int  enet_ioctl_ethsw_dos_ctrl(struct ethswctl_data *e);

int enet_arl_read( uint8_t *mac, uint32_t *vid, uint32_t *val );
int enet_arl_access_dump(void);
void enet_arl_dump_multiport_arl(void);
int remove_arl_entry(uint8_t *mac);
int remove_arl_entry_ext(uint8_t *mac);
int bcmeapi_init_ext_sw_if(extsw_info_t *extSwInfo);
#define bcmeapi_ioctl_ethsw_get_port_emac(e) 0 
#define bcmeapi_ioctl_ethsw_clear_port_emac(e) 0 
#define bcmeapi_ioctl_que_mon(pDevCtrl, e) 0;
#define bcmeapi_ioctl_que_map(pDevCtrl, e) 0;

void ephy_adjust_afe(unsigned int phy_id);

#endif /* _BCMSW_DMA_H_ */
