#!/bin/sh

#echo /sbin/hotplug-script > /proc/sys/kernel/hotplug
#insmod /lib/modules/3.4.11-rt19/kernel/drivers/net/wireless/quantenna/qdpc-host.ko

# echo remote rpc service ip addr to configure file.
echo "169.254.1.2" >/var/qcsapi_target_ip.conf

sleep 10
ifconfig host0 up
brctl addif br0 host0
ifconfig br0:host0 169.254.1.1 netmask 255.255.255.248

echo "add ebtables rule for QTN"
ebtables -D INPUT -p ARP --arp-ip-src 169.254.1.2 --in-if ! host0 -j DROP
ebtables -A INPUT -p ARP --arp-ip-src 169.254.1.2 --in-if ! host0 -j DROP
ebtables -D INPUT -p IPv4 --ip-src 169.254.1.2 --in-if ! host0 -j DROP
ebtables -A INPUT -p IPv4 --ip-src 169.254.1.2 --in-if ! host0 -j DROP
ebtables -D OUTPUT -p ARP --arp-ip-src 169.254.1.1 --out-if ! host0 -j DROP
ebtables -A OUTPUT -p ARP --arp-ip-src 169.254.1.1 --out-if ! host0 -j DROP
ebtables -D OUTPUT -p IPv4 --ip-src 169.254.1.1 --out-if ! host0 -j DROP
ebtables -A OUTPUT -p IPv4 --ip-src 169.254.1.1 --out-if ! host0 -j DROP
