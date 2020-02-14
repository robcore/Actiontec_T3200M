#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on
# 1. build server  (as do_act_build or do_act_release etc)
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk,

## section to dump info from DUT

echo "------Switch start-------"
echo "###### ethswctl -c arldump ######"
ethswctl -c arldump
echo "###### ethswctl -c pagedump -p 1 ######"
ethswctl -c pagedump -p 1
echo "###### ethswlct -c mibdump -p port ######"
for i in 0 1 2 3 4 5 6 7 8
do
ethswctl -c mibdump -p $i
done
echo "###### mdkshell stat port ######"
for i in 0 1 2 3 4 5 6 7 8
do
(echo "stat $i"; echo "exit") | mdkshell 2> /dev/null
done
echo "###### mdkshell phy port ######"
for i in 0 1 2 3
do
(echo "phy $i"; echo "exit") | mdkshell 2> /dev/null
done
echo "###### mdkshell port pause 0-8 ######"
(echo "port pause 0-8"; echo "exit") | mdkshell 2> /dev/null
echo "###### dw global port control register ######"
dw 0x80080000 9
echo "###### dw port vlan control register ######"
dw 0x8008c400 18
echo "###### dw pause capability register ######"
dw 0x800800a0
echo "###### dw Crossbar Switch Control Register ######"
dw 0x800c00ac
echo "###### ethctl eth0 media-type ######"
ethctl eth0 media-type
echo "###### ethctl eth1 media-type ######"
ethctl eth1 media-type
echo "###### ethctl eth2 media-type ######"
ethctl eth2 media-type
echo "###### ethctl eth3 media-type ######"
ethctl eth3 media-type
echo "###### ethctl eth4 media-type ######"
ethctl eth4 media-type
echo "------Switch end-------"
