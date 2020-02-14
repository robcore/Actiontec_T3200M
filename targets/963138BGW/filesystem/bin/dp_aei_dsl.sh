#!/bin/sh
# Copyright (c) 1993-2016 Actiontec Electronics, Inc
# try to define a script to run on 
# 1. build server  (as do_act_build or do_act_release etc) 
# 2. DUT  e.g. nvram show if bcm,  how about QTN, and RTL, QCA chip?  
# to retrieve info we want to gather, so we have a text based output for each SDK, each customer, to see customer profile info in old/new sdk, 

## section to dump info from DUT

echo "------DSL start-------"
xdslctl --version
/bin/xtm bonding --status
xdslctl0 profile --show
xdslctl1 profile --show
xdslctl0 info --cfg
xdslctl1 info --cfg
xdslctl0 info --stats
xdslctl1 info --stats
xdslctl0 info --pbParams
xdslctl1 info --pbParams
/bin/xtm bonding --status
dslcnt=1; while [ $dslcnt -le 10 ]; do /bin/echo "----trial $dslcnt seconds----"; xdslctl0 info --show; /bin/echo "-------------"; xdslctl1 info --show; /bin/xtm operate intf --stats; dslcnt=`expr $dslcnt + 1`; sleep 1; done
echo "------DSL end-------"
