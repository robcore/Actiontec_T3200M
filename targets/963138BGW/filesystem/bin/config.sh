#! /bin/sh

mkdir /var/bin
mkdir /var/etc

cp /bin/data_center /var/bin/
cp /etc/protype.xml /var/etc
cp /etc/cfg.xml /var/etc/
/var/bin/data_center&
