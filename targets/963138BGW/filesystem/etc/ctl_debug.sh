#! /bin/sh

# dbus
#mkdir /var/dbus

# data center
mkdir /var/bin
mkdir /var/etc
#cp /bin/data_center /var/bin/
cp /bin/tr69 /var/bin/
cp /etc/protype.xml /tmp/
#cp /etc/cfg.xml /tmp/

#
# startup
#

# dbus would be launched by SSK before this script, because several BRCM Daemons also depend on it now.
#rm /var/dbus/messagebus.pid
#cd /bin
#./dbus-daemon --system 	# depend on nothing


/var/bin/tr69 & 		# must start before data_center


# the daemon below depend on dbus, so sleep 10 sec here
#sleep 8

/tmp/rtd & 		# depend on dbus/SMD/MDM

/bin/pmd & 		# depend on dbus

sleep 10

/tmp/data_center & 	# depend on dbus/RTD
