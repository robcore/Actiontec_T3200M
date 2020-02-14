#!/bin/sh

trap "" 2


case "$1" in
	start)
		echo "Loading drivers and kernel modules... "
		echo

insmod /lib/modules/3.4.11-rt19/extra/wlcsm.ko 
# RDPA
insmod /lib/modules/3.4.11-rt19/extra/bdmf.ko bdmf_chrdev_major=215
insmod /lib/modules/3.4.11-rt19/extra/rdpa_gpl.ko 
insmod /lib/modules/3.4.11-rt19/extra/rdpa.ko 
 
insmod /lib/modules/3.4.11-rt19/extra/rdpa_mw.ko 
# General
insmod /lib/modules/3.4.11-rt19/extra/chipinfo.ko 
insmod /lib/modules/3.4.11-rt19/extra/bcmxtmrtdrv.ko 
insmod /lib/modules/3.4.11-rt19/extra/pktflow.ko 
insmod /lib/modules/3.4.11-rt19/extra/bcmxtmcfg.ko 
insmod /lib/modules/3.4.11-rt19/extra/adsldd.ko 
# enet depends on moca depends on i2c
 
insmod /lib/modules/3.4.11-rt19/extra/bcm_enet.ko 
# moving pktrunner after bcm_enet to get better FlowCache ICache performance
insmod /lib/modules/3.4.11-rt19/extra/pktrunner.ko 
insmod /lib/modules/3.4.11-rt19/extra/nciTMSkmod.ko 
 
#load SATA/AHCI modules
insmod /lib/modules/3.4.11-rt19/kernel/drivers/ata/libata.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/ata/libahci.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/ata/ahci.ko 
insmod /lib/modules/3.4.11-rt19/kernel/arch/arm/plat-bcm63xx/bcm63xx_sata.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/ata/ahci_platform.ko 
 
# pcie configuration save/restore
# WLAN accelerator module
insmod /lib/modules/3.4.11-rt19/extra/wfd.ko 
 
#WLAN Module
insmod /lib/modules/3.4.11-rt19/extra/wlemf.ko 
insmod /lib/modules/3.4.11-rt19/extra/dhd.ko firmware_path=/etc/wlan/dhd mfg_firmware_path=/etc/wlan/dhd/mfg
insmod /lib/modules/3.4.11-rt19/extra/wl.ko 
insmod /lib/modules/3.4.11-rt19/extra/dect.ko 
# T3200BMV T3200BV F3500 C3000 T3250V T3255V support VOIP
boardid=$(cat /proc/nvram/boardid)
VOIP_BOARD_IDs="T3200BMV T3200BV F3500 C3000 T3250V T3255V T3200MV"
for each in $VOIP_BOARD_IDs; do
if [ "$boardid" = $each ]; then
echo "$boardid loading voip drivers ...."
insmod /lib/modules/3.4.11-rt19/extra/dectshim.ko 
insmod /lib/modules/3.4.11-rt19/extra/pcmshim.ko 
insmod /lib/modules/3.4.11-rt19/extra/endpointdd.ko 
fi 
done 
 
#load usb modules
insmod /lib/modules/3.4.11-rt19/kernel/drivers/usb/host/ehci-hcd.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/usb/host/ohci-hcd.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/usb/host/xhci-hcd.ko 
insmod /lib/modules/3.4.11-rt19/kernel/arch/arm/plat-bcm63xx/bcm63xx_usb.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/usb/class/usblp.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/usb/storage/usb-storage.ko 
 
# other modules
 
insmod /lib/modules/3.4.11-rt19/extra/bcmvlan.ko 
insmod /lib/modules/3.4.11-rt19/extra/pwrmngtd.ko 
 
 
# presecure fullsecure modules
insmod /lib/modules/3.4.11-rt19/extra/otp.ko 
 
 
# RDPA Command Drivers
insmod /lib/modules/3.4.11-rt19/extra/rdpa_cmd.ko 
 
# OCF and PKA modules
 
# LTE PCIE driver module
 
#watchdog 
insmod /lib/modules/3.4.11-rt19/extra/bcm963138wdt.ko 
 
# Sensor driver
insmod /lib/modules/3.4.11-rt19/kernel/drivers/misc/lis3lv02d/lis3lv02d.ko 
insmod /lib/modules/3.4.11-rt19/kernel/drivers/misc/lis3lv02d/lis3lv02d_spi.ko 
echo gpon_i2c 0x50 > /sys/bus/i2c/devices/i2c-0/new_device
echo gpon_i2c 0x51 > /sys/bus/i2c/devices/i2c-0/new_device
echo gpon_i2c 0x52 > /sys/bus/i2c/devices/i2c-0/new_device

 test -e /etc/rdpa_init.sh && /etc/rdpa_init.sh


# Enable the PKA driver.
 test -e /sys/devices/platform/bcm_pka/enable && echo 1 > /sys/devices/platform/bcm_pka/enable

exit 0
		;;

	stop)
		echo "removing bcm base drivers not implemented yet..."
		exit 1
		;;

	*)
		echo "bcmbasedrivers: unrecognized option $1"
		exit 1
		;;

esac


