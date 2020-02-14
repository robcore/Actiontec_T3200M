#!/bin/sh

#Now use /sbin/hotplug
#echo /sbin/hotplug-script > /proc/sys/kernel/hotplug
insmod /lib/modules/3.4.11-rt19/kernel/drivers/net/wireless/quantenna/qdpc-host.ko

