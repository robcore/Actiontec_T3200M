#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on 
# 1. build server  (as do_act_build or do_act_release etc) 
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?  
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk, 

###########Section 1: wireless info #############
### Run the script to dump wireless default, runtime values
### some items as PA value, tx power/country codes,  frameburst if bcm, ACI mitigation, ampdu_density if bcm, zero config, 
### smart steering stats, acs, dfs re-entry, video pairing protection, aei reload if chip crash, 

## section to dump info from DUT
# QTN WiFi
# BCM WiFi

dump_bcm_wifi(){
    echo "===============nvram show=============="
    nvram show
    echo "===============wl ver=============="
    wl ver
    echo "===============wl status=============="
    wl status
    echo "===============wl country=============="
    wl country
    echo "===============cat zero config xml file=============="
    cat /var/wirelesssettings.xml
}

dump_qtn_wifi(){
    echo "===============qtn global configuration=============="
    echo "qcsapi_sockrpc get_bootcfg_param calstate"
    qcsapi_sockrpc get_bootcfg_param calstate
    echo "qcsapi_sockrpc is_startprod_done"
    qcsapi_sockrpc is_startprod_done
    echo "qcsapi_sockrpc get_primary_interface"
    qcsapi_sockrpc get_primary_interface
    echo "qcsapi_sockrpc get_regulatory_region wifi0"
    qcsapi_sockrpc get_regulatory_region wifi0
    echo "qcsapi_sockrpc get_mode wifi0"
    qcsapi_sockrpc get_mode wifi0
    echo "===============qtn radio level configuration=============="
    echo "qcsapi_sockrpc rfstatus wifi0"
    qcsapi_sockrpc rfstatus wifi0
    echo "qcsapi_sockrpc get_phy_mode wifi0"
    qcsapi_sockrpc get_phy_mode wifi0
    echo "qcsapi_sockrpc get_vht wifi0"
    qcsapi_sockrpc get_vht wifi0
    echo "qcsapi_sockrpc get_scs_status wifi0"
    qcsapi_sockrpc get_scs_status wifi0
    echo "qcsapi_sockrpc get_csw_records wifi0"
    qcsapi_sockrpc get_csw_records wifi0
    echo "qcsapi_sockrpc get_channel wifi0"
    qcsapi_sockrpc get_channel wifi0
    echo "qcsapi_sockrpc get_option wifi0 WMM"
    qcsapi_sockrpc get_option wifi0 WMM
    echo "qcsapi_sockrpc get_beacon_interval wifi0"
    qcsapi_sockrpc get_beacon_interval wifi0
    echo "qcsapi_sockrpc get_dtim wifi0"
    qcsapi_sockrpc get_dtim wifi0
    echo "qcsapi_sockrpc get_rts_threshold wifi0"
    qcsapi_sockrpc get_rts_threshold wifi0
    echo "qcsapi_sockrpc get_option wifi0 802_11h"
    qcsapi_sockrpc get_option wifi0 802_11h
    echo "qcsapi_sockrpc get_option wifi0 short_GI"
    qcsapi_sockrpc get_option wifi0 short_GI
    echo "qcsapi_sockrpc get_airfair wifi0"
    qcsapi_sockrpc get_airfair wifi0
    echo "qcsapi_sockrpc get_option wifi0 autorate"
    qcsapi_sockrpc get_option wifi0 autorate
    echo "qcsapi_sockrpc get_mcs_rate wifi0"
    qcsapi_sockrpc get_mcs_rate wifi0
    echo "qcsapi_sockrpc get_chan_power_table wifi0 curChannel"
    curChannel=`qcsapi_sockrpc get_channel wifi0`
    qcsapi_sockrpc get_chan_power_table wifi0 $curChannel
    echo "=============================================================="
    echo "===============qtn bss level wifi0 configuration=============="
    echo "qcsapi_sockrpc get_status wifi0"
    qcsapi_sockrpc get_status wifi0
    echo "qcsapi_sockrpc get_mac_addr wifi0"
    qcsapi_sockrpc get_mac_addr wifi0
    echo "qcsapi_sockrpc get_ssid wifi0"
    qcsapi_sockrpc get_ssid wifi0
    echo "qcsapi_sockrpc get_option wifi0 SSID_broadcast"
    qcsapi_sockrpc get_option wifi0 SSID_broadcast
    echo "qcsapi_sockrpc get_beacon wifi0"
    qcsapi_sockrpc get_beacon wifi0
    echo "qcsapi_sockrpc get_WPA_encryption_modes wifi0"
    qcsapi_sockrpc get_WPA_encryption_modes wifi0
    echo "qcsapi_sockrpc get_key_passphrase wifi0 0"
    qcsapi_sockrpc get_key_passphrase wifi0 0
    echo "qcsapi_sockrpc get_macaddr_filter wifi0"
    qcsapi_sockrpc get_macaddr_filter wifi0
    echo "qcsapi_sockrpc get_authorized_macaddr wifi0"
    qcsapi_sockrpc get_authorized_macaddr wifi0
    echo "qcsapi_sockrpc get_denied_macaddr wifi0"
    qcsapi_sockrpc get_denied_macaddr wifi0
    echo "qcsapi_sockrpc get_bss_isolate wifi0"
    qcsapi_sockrpc get_bss_isolate wifi0
    echo "qcsapi_sockrpc get_intra_bss_isolate wifi0"
    qcsapi_sockrpc get_intra_bss_isolate wifi0
    echo "qcsapi_sockrpc get_priority wifi0"
    qcsapi_sockrpc get_priority wifi0
    echo "qcsapi_sockrpc get_dscp_ac_map wifi0"
    qcsapi_sockrpc get_dscp_ac_map wifi0
    echo "qcsapi_sockrpc get_wps_access_control wifi0"
    qcsapi_sockrpc get_wps_access_control wifi0
    echo "qcsapi_sockrpc registrar_get_pp_devname wifi0 blacklist"
    qcsapi_sockrpc registrar_get_pp_devname wifi0 blacklist
    echo "qcsapi_sockrpc registrar_get_pp_devname wifi0 whitelist"
    qcsapi_sockrpc registrar_get_pp_devname wifi0 whitelist
    echo "qcsapi_sockrpc get_pairing_enable wifi0"
    qcsapi_sockrpc get_pairing_enable wifi0
    echo "qcsapi_sockrpc get_pairing_id wifi0"
    qcsapi_sockrpc get_pairing_id wifi0
    echo "qcsapi_sockrpc show_vlan_config wifi0"
    qcsapi_sockrpc show_vlan_config wifi0
    echo "qcsapi_sockrpc get_wps_state wifi0"
    qcsapi_sockrpc get_wps_state wifi0
    echo "qcsapi_sockrpc get_wps_param wifi0 ap_pin_fail_method"
    qcsapi_sockrpc get_wps_param wifi0 ap_pin_fail_method
    echo "qcsapi_sockrpc get_wps_param wifi0 ap_setup_locked"
    qcsapi_sockrpc get_wps_param wifi0 ap_setup_locked
    echo "qcsapi_sockrpc get_wps_param wifi0 auto_lockdown_max_retry"
    qcsapi_sockrpc get_wps_param wifi0 auto_lockdown_max_retry
    echo "qcsapi_sockrpc get_wps_param wifi0 auto_lockdown_fail_num"
    qcsapi_sockrpc get_wps_param wifi0 auto_lockdown_fail_num
    echo "=============================================================="
    echo "===============qtn bss level wifi3 configuration=============="
    echo "qcsapi_sockrpc get_status wifi3"
    qcsapi_sockrpc get_status wifi3
    echo "qcsapi_sockrpc get_mac_addr wifi3"
    qcsapi_sockrpc get_mac_addr wifi3
    echo "qcsapi_sockrpc get_ssid wifi3"
    qcsapi_sockrpc get_ssid wifi3
    echo "qcsapi_sockrpc get_option wifi3 SSID_broadcast"
    qcsapi_sockrpc get_option wifi3 SSID_broadcast
    echo "qcsapi_sockrpc get_beacon wifi3"
    qcsapi_sockrpc get_beacon wifi3
    echo "qcsapi_sockrpc get_WPA_encryption_modes wifi3"
    qcsapi_sockrpc get_WPA_encryption_modes wifi3
    echo "qcsapi_sockrpc get_key_passphrase wifi3 0"
    qcsapi_sockrpc get_key_passphrase wifi3 0
    echo "qcsapi_sockrpc get_macaddr_filter wifi3"
    qcsapi_sockrpc get_macaddr_filter wifi3
    echo "qcsapi_sockrpc get_authorized_macaddr wifi3"
    qcsapi_sockrpc get_authorized_macaddr wifi3
    echo "qcsapi_sockrpc get_denied_macaddr wifi3"
    qcsapi_sockrpc get_denied_macaddr wifi3
    echo "qcsapi_sockrpc get_bss_isolate wifi3"
    qcsapi_sockrpc get_bss_isolate wifi3
    echo "qcsapi_sockrpc get_intra_bss_isolate wifi3"
    qcsapi_sockrpc get_intra_bss_isolate wifi3
    echo "qcsapi_sockrpc get_priority wifi3"
    qcsapi_sockrpc get_priority wifi3
    echo "qcsapi_sockrpc get_dscp_ac_map wifi3"
    qcsapi_sockrpc get_dscp_ac_map wifi3
    echo "qcsapi_sockrpc get_wps_access_control wifi3"
    qcsapi_sockrpc get_wps_access_control wifi3
    echo "qcsapi_sockrpc registrar_get_pp_devname wifi3 blacklist"
    qcsapi_sockrpc registrar_get_pp_devname wifi3 blacklist
    echo "qcsapi_sockrpc registrar_get_pp_devname wifi3 whitelist"
    qcsapi_sockrpc registrar_get_pp_devname wifi3 whitelist
    echo "qcsapi_sockrpc get_pairing_enable wifi3"
    qcsapi_sockrpc get_pairing_enable wifi3
    echo "qcsapi_sockrpc get_pairing_id wifi3"
    qcsapi_sockrpc get_pairing_id wifi3
    echo "qcsapi_sockrpc show_vlan_config wifi3"
    qcsapi_sockrpc show_vlan_config wifi3
    echo "qcsapi_sockrpc get_wps_state wifi3"
    qcsapi_sockrpc get_wps_state wifi3
    echo "qcsapi_sockrpc wps_on_hidden_ssid_status wifi3"
    qcsapi_sockrpc wps_on_hidden_ssid_status wifi3
    echo "qcsapi_sockrpc get_wps_param wifi3 ap_pin_fail_method"
    qcsapi_sockrpc get_wps_param wifi3 ap_pin_fail_method
    echo "qcsapi_sockrpc get_wps_param wifi3 ap_setup_locked"
    qcsapi_sockrpc get_wps_param wifi3 ap_setup_locked
    echo "qcsapi_sockrpc get_wps_param wifi3 auto_lockdown_max_retry"
    qcsapi_sockrpc get_wps_param wifi3 auto_lockdown_max_retry
    echo "qcsapi_sockrpc get_wps_param wifi3 auto_lockdown_fail_num"
    qcsapi_sockrpc get_wps_param wifi3 auto_lockdown_fail_num
    echo "===============cat qtn fw reload times=============="
    cat /var/has_qtn
}

if [ $# -ne 1 ]; then
    echo "===============dump current BRCM wireless important info=============="
    dump_bcm_wifi
    echo "===============dump current QTN wireless important info=============="
    dump_qtn_wifi
else
    if [ $1 == "BRCM" ]; then
        echo "===============dump current BRCM wireless important info=============="
        dump_bcm_wifi
    else
        if [ $1 == "QTN" ]; then
             echo "===============dump current QTN wireless important info=============="
             dump_qtn_wifi
        else
            echo "usage:"
            echo "dp_aei_wifi.sh        ==>  dump current BRCM&QTN wireless important info"
            echo "dp_aei_wifi.sh BRCM   ==>  dump current BRCM wireless important info"
            echo "dp_aei_wifi.sh QTN    ==>  dump current QTN wireless important info"
        fi
    fi

fi
