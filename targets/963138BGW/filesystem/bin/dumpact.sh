#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on 
# 1. build server  (as do_act_build or do_act_release etc) 
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?  
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk, 

## section to dump info from DUT

dp_aei_dsl.sh >/tmp/dumpact.log 2>&1
dp_aei_system.sh >>/tmp/dumpact.log 2>&1
dp_aei_wifi.sh >>/tmp/dumpact.log 2>&1
dp_aei_pvr.sh >>/tmp/dumpact.log 2>&1
dp_aei_abs.sh >>/tmp/dumpact.log 2>&1
dp_aei_switch.sh >>/tmp/dumpact.log 2>&1
