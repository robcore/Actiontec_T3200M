/*
    Copyright 2004-2013 Broadcom Corporation

    <:label-BRCM:2013:DUAL/GPL:standard
    
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

#ifndef _ETHSW_PWRMNGT_H_
#define _ETHSW_PWRMNGT_H_

int    ethsw_shutdown_unused_phys(void);
int    ethsw_shutdown_unused_macs(BcmEnet_devctrl *pVnetDev0);
void   ethsw_setup_hw_apd(unsigned int enable);
int    ethsw_phy_pll_up(int ephy_and_gphy);
uint32 ethsw_ephy_auto_power_down_wakeup(void);
uint32 ethsw_ephy_auto_power_down_sleep(void);
void   ethsw_switch_manage_port_power_mode(int portnumber, int power_mode);
int    ethsw_switch_get_port_power_mode(int portnumber);

void   ethsw_eee_enable_phy(int log_port);
void   ethsw_eee_init(void);
void   ethsw_eee_port_enable(int port, int enable, int linkstate);
void   ethsw_eee_process_delayed_enable_requests(void);

void   extsw_apd_set_compatibility_mode(void);
void   ethsw_eee_init_hw(void);
void   extsw_eee_init(void);


#if defined(GPL_CODE_POWERSAVE)
void AEI_ethsw_disable_pws(void);
void AEI_ethsw_set_pws_mode(enum PowerSaveMode mode, UBOOL8 enable);
#endif

void   ethsw_deep_green_mode_handler(int linkState);

int BcmPwrMngtGetEthAutoPwrDwn(void);
void BcmPwrMngtSetEthAutoPwrDwn(unsigned int enable, int linkState);
void BcmPwrMngtSetEnergyEfficientEthernetEn(unsigned int enable);
int BcmPwrMngtGetEnergyEfficientEthernetEn(void);

int  BcmPwrMngtGetDeepGreenMode(int mode);
void BcmPwrMngtSetDeepGreenMode(unsigned int enable, int linkState);

#endif
