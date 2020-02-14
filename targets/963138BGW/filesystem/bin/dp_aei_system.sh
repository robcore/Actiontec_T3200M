#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on 
# 1. build server  (as do_act_build or do_act_release etc) 
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?  
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk, 

## section to dump info from DUT

echo "------SYSTEM start-------"
echo "Interrupts:"
cat /proc/interrupts
echo "..."
sleep 5
cat /proc/interrupts
echo "..."
sleep 5
cat /proc/interrupts

echo "Interfaces:"
ifconfig -a
echo "vmstat:"
cat /proc/vmstat
echo "slabinfo:"
cat /proc/slabinfo
echo "meminfo:"
cat /proc/meminfo
echo "ps:"
ps
echo "cat /proc/uptime"
cat /proc/uptime
echo "Bridge info:"
brctl show
echo "MAC addresses:"
brctl showmacs br0
echo "ARP entry"
cat /proc/net/arp
echo "dmesg:"
dmesg
echo "ls /var/core.* -l"
ls /var/core.* -l
echo "------SYSTEM end-------"
