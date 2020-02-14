#! /bin/sh

str=0
main()
{
#	str=$(cwmp -g Device.X_TWC_COM_RemoteAdministration.Telnet.NetworkAccess)
    str=$(cli -gpv InternetGatewayDevice.X_BROADCOM_COM_AppCfg.TelnetdCfg.NetworkAccess)
#	echo ${str}

	if [ -z "$(echo ${str} | grep "LAN")" ]; then
#       cwmp -s Device.X_TWC_COM_RemoteAdministration.Telnet.NetworkAccess string "LAN"
        cli -spv InternetGatewayDevice.X_BROADCOM_COM_AppCfg.TelnetdCfg.NetworkAccess string "LAN"
		echo "Enable LAN telnet"
	else
		echo "LAN telnet was enabled"
	fi
}

main
