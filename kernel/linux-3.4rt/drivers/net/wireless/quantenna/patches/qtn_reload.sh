#!/bin/sh

#reload quantenna host driver
echo "Ready to unload qdpc-host driver"
rmmod qdpc_host

#MUST sleep more than 2 seconds, otherwise load qdpc-host driver will cause MDIO write crash after only first qtn reboot.
sleep 2

echo "Ready to load qdpc-host driver"
insmod /lib/modules/3.4.11-rt19/kernel/drivers/net/wireless/quantenna/qdpc-host.ko

#config host0
/sbin/q11ac.sh

