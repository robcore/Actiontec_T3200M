#! /bin/sh

# dbus
#mkdir /var/dbus

# data center
mkdir /var/etc
cp /etc/protype.xml /tmp/
#cp /etc/cfg.xml /tmp/

#
# startup
#

# dbus would be launched by SSK before this script, because several BRCM Daemons also depend on it now.
#rm /var/dbus/messagebus.pid
#cd /bin
#./dbus-daemon --system 	# depend on nothing


#cd /var/bin/
#cd /bin/
#./tr69 & 		# must start before data_center


# the daemon below depend on dbus, so sleep 10 sec here
#sleep 8

cd /bin/
#./rtd & 		# depend on dbus/SMD/MDM
./pmd & 		# depend on dbus

sleep 10

cd /bin/
./data_center & 	# depend on dbus/RTD

#start csshd
#if [ -x "/bin/csshd" ]; then
 #   /bin/csshd
#fi
