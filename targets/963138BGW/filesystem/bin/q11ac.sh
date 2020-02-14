#!/bin/sh

qtnnetmask=""
if [ "$*" = "" ]; then
  echo "usage: q11ac.sh RGMII/PCIE"
  echo "use default config"
fi

# echo remote rpc service ip addr to configure file.
echo "169.254.1.2" >/var/qcsapi_target_ip.conf

if [ -e /bin/mp_is_manu_mode.sh ]; then
  /bin/mp_is_manu_mode.sh
  if [ $? -eq 0 ]; then
    qtnnetmask="255.255.255.252"
  else
    qtnnetmask="255.255.255.248"
  fi
else
  qtnnetmask="255.255.255.252"
fi

if [ "$1" = "RGMII" ]; then
  sleep 1
  echo "config RGMII 11AC"
  ifconfig eth4 up
  if [ "$2" != "eth4.0" ]; then
     brctl addif br0 eth4 #When LAN_VLAN Enabled, should not add it to br0
  fi
  ifconfig br0:eth 169.254.1.1 netmask ${qtnnetmask}
else
  if [ "$1" = "PCIE" ]; then
    sleep 10
    echo "config PCIE 11AC"
    ifconfig host0 up
    if [ "$2" != "host0.0" ]; then
        brctl addif br0 host0 #When LAN_VLAN Enabled, should not add it to br0
    fi
    ifconfig br0:host0 169.254.1.1 netmask ${qtnnetmask}
  else
    echo "use default config(RGMII)"
    ifconfig eth4 up
    if [ "$2" != "eth4.0" ]; then
        brctl addif br0 eth4 #When LAN_VLAN Enabled, should not add it to br0
    fi
    ifconfig br0:eth 169.254.1.1 netmask ${qtnnetmask}
  fi
fi

dev=$2
if [ "$dev" = "" ]; then
   #dev="eth4.0" //LAN_VLAN Enalbe
   dev="eth4"
fi
echo "add ebtables rule for QTN"
ebtables -D INPUT -p ARP --arp-ip-src 169.254.1.2 --in-if ! $dev -j DROP
ebtables -A INPUT -p ARP --arp-ip-src 169.254.1.2 --in-if ! $dev -j DROP
ebtables -D INPUT -p IPv4 --ip-src 169.254.1.2 --in-if ! $dev -j DROP
ebtables -A INPUT -p IPv4 --ip-src 169.254.1.2 --in-if ! $dev -j DROP
ebtables -D OUTPUT -p ARP --arp-ip-src 169.254.1.1 --out-if ! $dev -j DROP
ebtables -A OUTPUT -p ARP --arp-ip-src 169.254.1.1 --out-if ! $dev -j DROP
ebtables -D OUTPUT -p IPv4 --ip-src 169.254.1.1 --out-if ! $dev -j DROP
ebtables -A OUTPUT -p IPv4 --ip-src 169.254.1.1 --out-if ! $dev -j DROP
ebtables -t nat -D PREROUTING -p ARP --arp-ip-src 169.254.1.2 ! --arp-ip-dst 169.254.1.1 -j DROP
ebtables -t nat -A PREROUTING -p ARP --arp-ip-src 169.254.1.2 ! --arp-ip-dst 169.254.1.1 -j DROP
ebtables -t nat -D PREROUTING -p IPv4 --ip-src 169.254.1.2 ! --ip-dst 169.254.1.1 -j DROP
ebtables -t nat -A PREROUTING -p IPv4 --ip-src 169.254.1.2 ! --ip-dst 169.254.1.1 -j DROP
ebtables -t nat -D PREROUTING -p ARP ! --arp-ip-src 169.254.1.1 --arp-ip-dst 169.254.1.2 -j DROP
ebtables -t nat -A PREROUTING -p ARP ! --arp-ip-src 169.254.1.1 --arp-ip-dst 169.254.1.2 -j DROP
ebtables -t nat -D PREROUTING -p IPv4 ! --ip-src 169.254.1.1 --ip-dst 169.254.1.2 -j DROP
ebtables -t nat -A PREROUTING -p IPv4 ! --ip-src 169.254.1.1 --ip-dst 169.254.1.2 -j DROP
