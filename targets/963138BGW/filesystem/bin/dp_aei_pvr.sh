#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on 
# 1. build server  (as do_act_build or do_act_release etc) 
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?  
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk, 

## section to dump info from DUT

echo "------PVR start-------"
echo "dumpmdm"
dumpmdm
echo "dumpcfg"
dumpcfg
echo "ps"
ps
echo "iptables -t nat -nvL"
iptables -t nat -nvL
echo "iptables -nvL"
iptables -nvL
echo "ebtables -L --Lc"
ebtables -L --Lc
echo "ebtables -t broute -L --Lc"
ebtables -t broute -L --Lc
echo "cat /proc/fcache/*"
cat /proc/fcache/*
echo "cat /var/mcpd.conf"
cat /var/mcpd.conf
echo "route  -n"
route  -n
echo "------PVR end-------"
