#!/bin/sh
#
# iptables-p2p example script
#
# Filipe Almeida <filipe@rnl.ist.utl.pt>
#

FW="/bin/iptables"
TC="/bin/tc"


P2P_MARK=10
IFACES="ewan0.1"

LINK_RATE="1mbit"
P2P_RATE="50kbit"

ROOT=1
NORMAL=2
P2P=3

$FW -t mangle -D PREROUTING -m p2p --p2p all -j CONNMARK --set-mark $P2P_MARK 2>/dev/null
$FW -t mangle -D PREROUTING -m connmark --mark $P2P_MARK -j CONNMARK --restore-mark 2>/dev/null
$FW -t mangle -A PREROUTING -m p2p --p2p all -j CONNMARK --set-mark $P2P_MARK
$FW -t mangle -A PREROUTING -m connmark --mark $P2P_MARK -j CONNMARK --restore-mark

for i in $IFACES
do
    $TC qdisc del dev $i root 2>/dev/null
    $TC qdisc add dev $i root handle 1: htb default $NORMAL
    $TC class add dev $i parent 1: classid 1:$ROOT htb rate $LINK_RATE ceil $LINK_RATE
    $TC class add dev $i parent 1:$ROOT classid 1:$NORMAL htb rate $LINK_RATE ceil $LINK_RATE
    $TC class add dev $i parent 1:$ROOT classid 1:$P2P htb rate $P2P_RATE ceil $P2P_RATE
    $TC filter add dev $i protocol ip prio 1 parent 1:0 \
        handle $P2P_MARK fw classid 1:$P2P
done

