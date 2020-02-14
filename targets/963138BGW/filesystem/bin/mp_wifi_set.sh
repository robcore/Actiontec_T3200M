#! /bin/sh

band=$1
standard=$2
option=$2
channel=$3
radio_enable=$3
bandwidth=$4
ssid=$5
security=$6
key=$7
txpower=$8
wps=$9

help_info(){
    echo "MFG setting for ${band} WIFI "
	echo "	Standard = ${standard}"
	echo "	Channel = ${channel}"
	echo "	Bandwidth = ${bandwidth}"
	echo "	SSID = ${ssid}"
	echo "	Security = ${security}"
	echo "	Key = ${key}"
	echo "	TxPower = ${txpower}"
	echo "	WPStatus = ${wps}"
	echo "setting begin ..."
}

set_5G_radio() {
        result=$(qcsapi_sockrpc get_bootcfg_param calstate | grep "1")
        if [ -n "${result}" ] ; then
            echo "calstate=1, need do calibration! ^_^"
            exit 1
        fi

        QTN_VER=$(qcsapi_sockrpc get_firmware_version)
	if [ "${QTN_VER:0:3}" == "v36" ]; then
                # ARC2.8
		qcsapi_sockrpc rfenable wifi0 $1
	else
		qcsapi_sockrpc rfenable $1
	fi
}

set_5g_wifi(){

        set_5G_radio 1
	#killall -15 wps_monitor
	#rm -f /tmp/wps_monitor.pid
	#set standard
	if [ "$standard" == "a,n,ac" ]
	then
		echo "qcsapi_sockrpc set_vht wifi0 1"
		qcsapi_sockrpc set_vht wifi0 1
	else
		echo "qcsapi_sockrpc set_vht wifi0 0"
		qcsapi_sockrpc set_vht wifi0 0
	fi
	#set channel
	if [ "$channel" -ge 36 ] && [ "$channel" -le 165 ]
	then
		echo "qcsapi_sockrpc enable_scs wifi0 0"
		qcsapi_sockrpc enable_scs wifi0 0
		echo "qcsapi_sockrpc set_channel wifi0 $channel"
		qcsapi_sockrpc set_channel wifi0 $channel
	else
		if [ "$channel" -eq 0 ]
		then
			echo "qcsapi_sockrpc enable_scs wifi0 1"
			qcsapi_sockrpc enable_scs wifi0 1
		fi
	fi
	#set bandwidth
	if [ "$bandwidth" -eq 80 -o "$bandwidth" -eq 40 -o "$bandwidth" -eq 20 ]
	then
		echo "qcsapi_sockrpc set_bw wifi0 $bandwidth"
		qcsapi_sockrpc set_bw wifi0 $bandwidth
	fi
	#set ssid
	echo "qcsapi_sockrpc set_ssid wifi0 $ssid"
	qcsapi_sockrpc set_ssid wifi0 $ssid
	#set security
	if [ "$security" == "None" ]
	then
		echo "qcsapi_sockrpc set_beacon wifi0 Basic"
		qcsapi_sockrpc set_beacon wifi0 Basic
	else
		echo "qcsapi_sockrpc set_beacon wifi0 11i"
		qcsapi_sockrpc set_beacon wifi0 11i
		echo "qcsapi_sockrpc set_WPA_encryption_modes wifi0 AESEncryption"
		qcsapi_sockrpc set_WPA_encryption_modes wifi0 AESEncryption
		echo "qcsapi_sockrpc set_key_passphrase wifi0 0 $key"
		qcsapi_sockrpc set_key_passphrase wifi0 0 $key
	fi
	#set txpower
	if [ "$txpower" -ge 10 ] && [ "$txpower" -le 100 ]
	then
		echo "qcsapi_sockrpc set_tx_power_ext wifi0 $txpower 1 2 17 18 18"
	fi
	#set wps
	if [ "$wps" == "enabled" ]
	then
		echo "qcsapi_sockrpc set_wps_configured_state wifi0 2"
		nvram set qtn_wps_mode=enabled
		qcsapi_sockrpc set_wps_configured_state wifi0 2
	else
		echo "qcsapi_sockrpc set_wps_configured_state wifi0 0"
		nvram set qtn_wps_mode=disabled
		qcsapi_sockrpc set_wps_configured_state wifi0 0
	fi
	#restart wps_monitor
	#wps_monitor&
}

set_2g_wifi_wl(){

	ifconfig wl0 up >> /dev/null
	brctl addif br0 wl0 >> /dev/null
	wl radio on
	# down 2.4G wifi interface first
	ifconfig wl0 down
	wl -i wl0 down
	wl -i wl0 bss -C 0 down
	#stop related deamons
	#killall -15 acsd
	killall -15 nas
	killall -15 eapd
	#killall -15 wps_monitor
	#rm -f /tmp/wps_monitor.pid
	#set standard
	case "$standard" in
	"b,g,n")
		echo "set standard b,g,n"
		wl -i wl0 nmode 3
		wl -i wl0 nreqd 0
		wl -i wl0 gmode 2
	;;

	"b,g")
		echo "set standard b,g"
		wl -i wl0 nmode 0
		wl -i wl0 nreqd 0
		wl -i wl0 gmode 1
	;;

	"g,n")
		echo "set standard g,n"
		wl -i wl0 nmode 3
		wl -i wl0 nreqd 0
		wl -i wl0 gmode 1
	;;

	"n")
		echo "set standard n"
		wl -i wl0 nmode 3
		wl -i wl0 nreqd 1
		wl -i wl0 gmode 0
	;;

	"g")
		echo "set standard g"
		wl -i wl0 nmode 0
		wl -i wl0 nreqd 0
		wl -i wl0 gmode 2
	;;

	"b")
		echo "set standard b"
		wl -i wl0 nmode 0
		wl -i wl0 nreqd 0
		wl -i wl0 gmode 0
	;;

	*)
		echo "error $standard";;
	esac

	#set channel
	if [ "$channel" -ge 1 ] && [ "$channel" -le 11 ]
	then
		echo "nvram set wl0_chanspec=$channel"
		wl -i wl0 channel $channel
		nvram set wl0_channel=$channel
		nvram set wl0_chanspec=$channel
	else
		if [ "$channel" -eq 0 ]
		then
			echo "nvram set wl0_chanspec=$channel"
			nvram set wl0_channel=$channel
			nvram set wl0_chanspec=$channel
		fi
	fi
	#set bandwidth
	if [ "$bandwidth" -eq 40 ]
	then
		echo "set bandwidth 40MHz"
		echo "wl -i wl0 chanspec -c $channel -b 2 -w 40 -s -1"
		wl -i wl0 bw_cap 2g 0x3
		wl -i wl0 chanspec -c $channel -b 2 -w 40 -s -1
		if [ $? -ne 0 ]
		then
			echo "wl -i wl0 chanspec -c $channel -b 2 -w 40 -s 1"
			wl -i wl0 chanspec -c $channel -b 2 -w 40 -s 1
		fi
	else
		echo "wl -i wl0 chanspec -c $channel -b 2 -w 20"
		wl -i wl0 bw_cap 2g 0x3
		wl -i wl0 chanspec -c $channel -b 2 -w 20
	fi
	#set ssid
	echo "set ssid=$ssid"
	nvram set wl0_ssid=$ssid
	wl -i wl0 ssid $ssid

	#set security
	if [ "$security" == "None" ]
	then
		echo "set security none"
		nvram set wl0_bss_enabled=1
		nvram set wl0_auth_mode=none
		nvram set wl0_wsec=0
		wl -i wl0 wsec 0
		wl -i wl0 wsec_restrict 0
		wl -i wl0 wpa_auth 0
		wl -i wl0 eap 0
		wl -i wl0 auth 0
	else
		echo "set security wpa2"
		nvram set wl0_bss_enabled=1
		nvram set wl0_auth_mode=none
		nvram set wl0_wpa_gtk_rekey=3600
		nvram set wl0_wpa_psk=$key
		nvram set wl0_akm="psk psk2"
		nvram set wl0_crypto=aes
		nvram set wl0_wsec=4
		wl -i wl0 wsec -C 0 4
		wl -i wl0 wsec_restrict -C 0 1
		wl -i wl0 wpa_auth -C 0 132
		wl -i wl0 eap 0
		wl -i wl0 auth -C 0 0
	fi
	#set txpower
	if [ "$txpower" -ge 10 ] && [ "$txpower" -le 100 ]
	then
		echo "wl -i wl0 pwr_percent $txpower"
		wl -i wl0 pwr_percent $txpower
	fi
	#set wps
	if [ "$wps" == "enabled" ]
	then
		echo "nvram set wl0_wps_mode=enabled"
		nvram set wl0_wps_mode=enabled
	else
		echo "nvram set wl0_wps_mode=disabled"
		nvram set wl0_wps_mode=disabled
	fi

	#it's time to make wl0 up
	wl -i wl0 up
	wl -i wl0 bss -C 0 up
	ifconfig wl0 up
	#launch related deamons
	#acsd&
	nas&
	eapd&
	#wps_monitor&
}

set_2g_wifi_cwmp(){
	#set standard
	if [ "$standard" == "n" ]
	then
		echo "cwmp -p Device.WiFi.Radio.1.X_BROADCOM_COM_WlNmode string auto -p  Device.WiFi.Radio.1.X_BROADCOM_COM_WlgMode int 1 -e"
		cwmp -p Device.WiFi.Radio.1.X_BROADCOM_COM_WlNmode string auto -p  Device.WiFi.Radio.1.X_BROADCOM_COM_WlgMode int 1 -e
	else
		if [ "$standard" == "g" ]
		then
			echo "cwmp -p Device.WiFi.Radio.1.X_BROADCOM_COM_WlNmode string off -p  Device.WiFi.Radio.1.X_BROADCOM_COM_WlgMode int 1 -e"
			cwmp -p Device.WiFi.Radio.1.X_BROADCOM_COM_WlNmode string off -p  Device.WiFi.Radio.1.X_BROADCOM_COM_WlgMode int 1 -e
		else
			if [ "$standard" == "b" ]
			then
				echo "cwmp -p Device.WiFi.Radio.1.X_BROADCOM_COM_WlNmode string off -p  Device.WiFi.Radio.1.X_BROADCOM_COM_WlgMode int 0 -e"
			fi
		fi
	fi
	    #set channel
	if [ "$channel" -ge 1 ] && [ "$channel" -le 11 ]
	then
		echo "cwmp -p Device.WiFi.Radio.1.Channel unsignedInt 6 -p Device.WiFi.Radio.1.AutoChannelEnable boolean 0 -e"
		cwmp -p Device.WiFi.Radio.1.Channel unsignedInt 6 -p Device.WiFi.Radio.1.AutoChannelEnable boolean 0 -e
	else
		if [ "$channel" -eq 0 ]
		then
			echo "cwmp -s Device.WiFi.Radio.1.AutoChannelEnable boolean 0"
			cwmp -s Device.WiFi.Radio.1.AutoChannelEnable boolean 0
		fi
	fi
	#set bandwidth
	if [ "$bandwidth" -eq 40 ]
	then
		echo "cwmp -s Device.WiFi.Radio.1.X_BROADCOM_COM_WlNBwCap int 2"
		cwmp -s Device.WiFi.Radio.1.X_BROADCOM_COM_WlNBwCap int 2
	else
		echo "cwmp -s Device.WiFi.Radio.1.X_BROADCOM_COM_WlNBwCap int 0"
		cwmp -s Device.WiFi.Radio.1.X_BROADCOM_COM_WlNBwCap int 0
	fi
	#set ssid
	echo "cwmp -s Device.WiFi.SSID.1.SSID string $ssid"
	cwmp -s Device.WiFi.SSID.1.SSID string $ssid
	#set security
	if [ "$security" == "None" ]
	then
		echo "cwmp -s Device.WiFi.AccessPoint.1.Security.ModeEnabled string None"
		cwmp -s Device.WiFi.AccessPoint.1.Security.ModeEnabled string None
	else
		echo "cwmp -p Device.WiFi.AccessPoint.1.Security.ModeEnabled string WPA-WPA2-Personal"
		echo "     -p Device.WiFi.AccessPoint.1.Security.KeyPassphrase string $key"
		echo "     -p Device.WiFi.AccessPoint.1.Security.WlWep string disabled"
		echo "     -p Device.WiFi.AccessPoint.1.Security.X_BROADCOM_COM_WlWpaEncryption string aes -e"

		cwmp -p Device.WiFi.AccessPoint.1.Security.ModeEnabled string WPA-WPA2-Personal \
		     -p Device.WiFi.AccessPoint.1.Security.KeyPassphrase string $key \
		     -p Device.WiFi.AccessPoint.1.Security.WlWep string disabled \
		     -p Device.WiFi.AccessPoint.1.Security.X_BROADCOM_COM_WlWpaEncryption string aes -e
	fi
	#set txpower
	if [ "$txpower" -ge 10 ] && [ "$txpower" -le 100 ]
	then
		echo "cwmp -s Device.WiFi.Radio.1.TransmitPower int $txpower"
		cwmp -s Device.WiFi.Radio.1.TransmitPower int $txpower
	fi
	#set wps
	if [ "$wps" == "enabled" ]
	then
		echo "cwmp -s Device.WiFi.AccessPoint.1.WPS.X_BROADCOM_COM_Wsc_mode string enabled"
		cwmp -s Device.WiFi.AccessPoint.1.WPS.X_BROADCOM_COM_Wsc_mode string enabled
	else
		echo "cwmp -s Device.WiFi.AccessPoint.1.WPS.X_BROADCOM_COM_Wsc_mode string disabled"
		cwmp -s Device.WiFi.AccessPoint.1.WPS.X_BROADCOM_COM_Wsc_mode string disabled
	fi
}

set_radio(){
	if [ "$band" == "5GHz" ]
	then
		if [ "$radio_enable" == "enable" ]
		then
			set_5G_radio 1
		else
			if [ "$radio_enable" == "disable" ]
			then
				set_5G_radio 0
			fi
		fi
	else
		if [ "$band" == "2.4GHz" ]
		then
			if [ "$radio_enable" == "enable" ]
			then
				ifconfig wl0 up >> /dev/null
				brctl addif br0 wl0 >> /dev/null
				wl radio on
			else
				if [ "$radio_enable" == "disable" ]
				then
					wl radio off
				fi
			fi
		else
			echo "Do not support this band!"
		fi
	fi
}

if [[ $# -ne 9 && $# -ne 3 ]]
then
        echo "usage:"
	echo "	mp_wifi_set.sh band Standard Channel Bandwidth SSID Security Key TxPower WPStatus"
	echo "	example: mp_wifi_set.sh 5GHz a,n,ac 157 80 ActWifi-5 WPA2 1234567890abcdef 100 disabled"
	echo "	example: mp_wifi_set.sh 2.4GHz b,g,n 6 40 ActWifi WPA2 1234567890abcdef 100 disabled"
	echo "	enable/disable 2.4GHz and 5GHz WIFI radio"
	echo "	example: mp_wifi_set.sh 2.4GHz radio enable"
	echo "	example: mp_wifi_set.sh 2.4GHz radio disable"
	echo "	example: mp_wifi_set.sh 5GHz radio enable"
	echo "	example: mp_wifi_set.sh 5GHz radio disable"
else
	if [[ $# -eq 3 && "$option" == "radio" ]]
	then
		set_radio
	else
		help_info
		if [ "$band" == "5GHz" ]
		then
			set_5g_wifi
		else
			if [ "$band" == "2.4GHz" ]
			then
				#set_2g_wifi_cwmp
				set_2g_wifi_wl
			else
				echo "Do not support this band!"
			fi
		fi
	fi
fi
