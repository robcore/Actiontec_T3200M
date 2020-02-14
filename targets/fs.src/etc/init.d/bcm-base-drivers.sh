#!/bin/sh

trap "" 2

#The following will be populated by buildFS during the make process:
KERNELVER=_set_by_buildFS_

if [ ! -d /lib/modules/$KERNELVER/extra ]; then
	echo "ERROR: bcm-base-drivers.sh: /lib/modules/$KERNELVER/extra does not exist" 1>&2
fi

case "$1" in
	start)
		echo "Loading drivers and kernel modules... "
		echo

 test -e /lib/modules/$KERNELVER/extra/bcm_log.ko && insmod /lib/modules/$KERNELVER/extra/bcm_log.ko
# RDPA
 test -e /lib/modules/$KERNELVER/extra/bdmf.ko && insmod /lib/modules/$KERNELVER/extra/bdmf.ko bdmf_chrdev_major=215 
 test -e /lib/modules/$KERNELVER/extra/gpon_stack.ko && insmod /lib/modules/$KERNELVER/extra/gpon_stack.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa_gpl.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_gpl.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && insmod /lib/modules/$KERNELVER/extra/rdpa.ko 
 test -e /etc/rdpa_init.sh && /etc/rdpa_init.sh
 
 test -e /lib/modules/$KERNELVER/extra/rdpa_mw.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_mw.ko
 test -e /lib/modules/$KERNELVER/extra/bcmbrfp.ko && insmod /lib/modules/$KERNELVER/extra/bcmbrfp.ko
# General
 test -e /lib/modules/$KERNELVER/extra/chipinfo.ko && insmod /lib/modules/$KERNELVER/extra/chipinfo.ko
 test -e /lib/modules/$KERNELVER/extra/bcmxtmrtdrv.ko && insmod /lib/modules/$KERNELVER/extra/bcmxtmrtdrv.ko
 test -e /lib/modules/$KERNELVER/extra/bcm_ingqos.ko && insmod /lib/modules/$KERNELVER/extra/bcm_ingqos.ko
 test -e /lib/modules/$KERNELVER/extra/bcm_bpm.ko && insmod /lib/modules/$KERNELVER/extra/bcm_bpm.ko
 test -e /lib/modules/$KERNELVER/extra/pktflow.ko && insmod /lib/modules/$KERNELVER/extra/pktflow.ko
 test -e /lib/modules/$KERNELVER/extra/pktcmf.ko && insmod /lib/modules/$KERNELVER/extra/pktcmf.ko
 test -e /lib/modules/$KERNELVER/extra/bcmfap.ko && insmod /lib/modules/$KERNELVER/extra/bcmfap.ko
 test -e /lib/modules/$KERNELVER/extra/pktrunner.ko && insmod /lib/modules/$KERNELVER/extra/pktrunner.ko
 test -e /lib/modules/$KERNELVER/extra/flowbond.ko && insmod /lib/modules/$KERNELVER/extra/flowbond.ko
 test -e /etc/cmf/cmfcfg && /etc/cmf/cmfcfg
 test -e /lib/modules/$KERNELVER/extra/profdrvdd.ko && insmod /lib/modules/$KERNELVER/extra/profdrvdd.ko
 test -e /lib/modules/$KERNELVER/extra/bcmxtmcfg.ko && insmod /lib/modules/$KERNELVER/extra/bcmxtmcfg.ko
 test -e /lib/modules/$KERNELVER/extra/ext_bonding.ko && insmod /lib/modules/$KERNELVER/extra/ext_bonding.ko
 test -e /lib/modules/$KERNELVER/extra/atmapi.ko && insmod /lib/modules/$KERNELVER/extra/atmapi.ko
 test -e /lib/modules/$KERNELVER/extra/adsldd.ko && insmod /lib/modules/$KERNELVER/extra/adsldd.ko
 test -e /lib/modules/$KERNELVER/extra/blaa_dd.ko && insmod /lib/modules/$KERNELVER/extra/blaa_dd.ko
 test -e /lib/modules/$KERNELVER/extra/bcmprocfs.ko && insmod /lib/modules/$KERNELVER/extra/bcmprocfs.ko
# test -e /lib/modules/$KERNELVER/kernel/net/ipv6/ipv6.ko && insmod /lib/modules/$KERNELVER/kernel/net/ipv6/ipv6.ko
 test -e /lib/modules/$KERNELVER/kernel/net/atm/br2684.ko && insmod /lib/modules/$KERNELVER/kernel/net/atm/br2684.ko
 test -e /lib/modules/$KERNELVER/extra/linux-kernel-bde.ko && insmod /lib/modules/$KERNELVER/extra/linux-kernel-bde.ko
 test -e /lib/modules/$KERNELVER/extra/linux-user-bde.ko && insmod /lib/modules/$KERNELVER/extra/linux-user-bde.ko
 # enet depends on moca depends on i2c
 test -e /lib/modules/$KERNELVER/extra/i2c_bcm6xxx.ko && insmod /lib/modules/$KERNELVER/extra/i2c_bcm6xxx.ko
 test -e /lib/modules/$KERNELVER/extra/bcm3450.ko && insmod /lib/modules/$KERNELVER/extra/bcm3450.ko
 test -e /lib/modules/$KERNELVER/extra/gpon_i2c.ko && insmod /lib/modules/$KERNELVER/extra/gpon_i2c.ko
 test -e /lib/modules/$KERNELVER/extra/gpon_i2c.ko && echo gpon_i2c 0x50 > /sys/bus/i2c/devices/i2c-0/new_device
 test -e /lib/modules/$KERNELVER/extra/gpon_i2c.ko && echo gpon_i2c 0x51 > /sys/bus/i2c/devices/i2c-0/new_device

 test -e /lib/modules/$KERNELVER/extra/laser_i2c.ko && insmod /lib/modules/$KERNELVER/extra/laser_i2c.ko
 test -e /lib/modules/$KERNELVER/extra/sfp_i2c.ko && insmod /lib/modules/$KERNELVER/extra/sfp_i2c.ko
 test -e /lib/modules/$KERNELVER/extra/bcmmoca.ko && insmod /lib/modules/$KERNELVER/extra/bcmmoca.ko
 test -e /lib/modules/$KERNELVER/extra/time_sync.ko && insmod /lib/modules/$KERNELVER/extra/time_sync.ko 
 test -e /lib/modules/$KERNELVER/extra/bcm_enet.ko && insmod /lib/modules/$KERNELVER/extra/bcm_enet.ko
 test -e /lib/modules/$KERNELVER/extra/nciTMSkmod.ko && insmod /lib/modules/$KERNELVER/extra/nciTMSkmod.ko
 test -e /lib/modules/$KERNELVER/extra/bcmsw.ko && insmod /lib/modules/$KERNELVER/extra/bcmsw.ko && ifconfig bcmsw up
 test -e /lib/modules/$KERNELVER/extra/bcm_usb.ko && insmod /lib/modules/$KERNELVER/extra/bcm_usb.ko
 test -e /lib/modules/$KERNELVER/extra/bcmarl.ko && insmod /lib/modules/$KERNELVER/extra/bcmarl.ko

#load SATA/AHCI modules
 test -e /lib/modules/$KERNELVER/kernel/drivers/ata/libata.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/ata/libata.ko
 test -e /lib/modules/$KERNELVER/kernel/drivers/ata/libahci.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/ata/libahci.ko
 test -e /lib/modules/$KERNELVER/kernel/drivers/ata/ahci.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/ata/ahci.ko
 test -e /lib/modules/$KERNELVER/kernel/arch/arm/plat-bcm63xx/bcm63xx_sata.ko && insmod /lib/modules/$KERNELVER/kernel/arch/arm/plat-bcm63xx/bcm63xx_sata.ko
 test -e /lib/modules/$KERNELVER/kernel/drivers/ata/ahci_platform.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/ata/ahci_platform.ko

#load usb modules
 for f in drivers/usb/usb-common drivers/usb/core/usbcore \
  drivers/usb/host/ehci-hcd drivers/usb/host/ohci-hcd drivers/usb/host/xhci-hcd \
  arch/arm/plat-bcm63xx/bcm63xx_usb \
  drivers/usb/class/usblp drivers/usb/storage/usb-storage; do
  test -e /lib/modules/$KERNELVER/kernel/$f.ko && insmod /lib/modules/$KERNELVER/kernel/$f.ko
 done

# pcie configuration save/restore
 test -e /lib/modules/$KERNELVER/kernel/arch/arm/plat-bcm63xx/bcm63xx_pcie.ko && insmod /lib/modules/$KERNELVER/kernel/arch/arm/plat-bcm63xx/bcm63xx_pcie.ko
# WLAN accelerator module
 test -e /lib/modules/$KERNELVER/extra/wfd.ko && insmod /lib/modules/$KERNELVER/extra/wfd.ko

#WLAN Module
 test -e /lib/modules/$KERNELVER/extra/wlcsm.ko && insmod /lib/modules/$KERNELVER/extra/wlcsm.ko
 test -e /lib/modules/$KERNELVER/extra/wlemf.ko && insmod /lib/modules/$KERNELVER/extra/wlemf.ko
 test -e /lib/modules/$KERNELVER/extra/dhd.ko && insmod /lib/modules/$KERNELVER/extra/dhd.ko  firmware_path=/etc/wlan/dhd mfg_firmware_path=/etc/wlan/dhd/mfg
 test -e /lib/modules/$KERNELVER/extra/wl.ko && insmod /lib/modules/$KERNELVER/extra/wl.ko
 test -e /etc/wlan/rtecdc.trx && mount -t usbfs none /proc/bus/usb && /bin/bcmdl /etc/wlan/rtecdc.trx && sleep 2 && insmod /lib/modules/$KERNELVER/extra/wl-usb.ko
 test -e /lib/modules/$KERNELVER/extra/dect.ko && insmod /lib/modules/$KERNELVER/extra/dect.ko 
 test -e /lib/modules/$KERNELVER/extra/dectshim.ko && insmod /lib/modules/$KERNELVER/extra/dectshim.ko 
 test -e /lib/modules/$KERNELVER/extra/dspdd.ko && insmod /lib/modules/$KERNELVER/extra/dspdd.ko
 test -e /lib/modules/$KERNELVER/extra/pcmshim.ko && insmod /lib/modules/$KERNELVER/extra/pcmshim.ko
 test -e /lib/modules/$KERNELVER/extra/endpointdd.ko && insmod /lib/modules/$KERNELVER/extra/endpointdd.ko
 test -e /lib/modules/$KERNELVER/extra/bt_serial_drv.ko && insmod /lib/modules/$KERNELVER/extra/bt_serial_drv.ko

# other modules
 test -e /lib/modules/$KERNELVER/extra/isdn.ko && insmod /lib/modules/$KERNELVER/extra/isdn.ko
 test -e /lib/modules/$KERNELVER/kernel/drivers/isdn/capi/capi.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/isdn/capi/capi.ko 
 test -e /lib/modules/$KERNELVER/extra/bcmgpon.ko && insmod /lib/modules/$KERNELVER/extra/bcmgpon.ko
 test -e /lib/modules/$KERNELVER/extra/bcmvlan.ko && insmod /lib/modules/$KERNELVER/extra/bcmvlan.ko
 test -e /lib/modules/$KERNELVER/extra/pwrmngtd.ko && insmod /lib/modules/$KERNELVER/extra/pwrmngtd.ko
 test -e /lib/modules/$KERNELVER/kernel/drivers/char/hw_random/rng-core.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/char/hw_random/rng-core.ko
 test -e /lib/modules/$KERNELVER/extra/bcmtrng.ko && insmod /lib/modules/$KERNELVER/extra/bcmtrng.ko

 test -e /lib/modules/$KERNELVER/extra/laser_dev.ko && insmod /lib/modules/$KERNELVER/extra/laser_dev.ko
 test -e /lib/modules/$KERNELVER/extra/pmd.ko && insmod /lib/modules/$KERNELVER/extra/pmd.ko
 test -e /lib/modules/$KERNELVER/extra/sim_card.ko && insmod /lib/modules/$KERNELVER/extra/sim_card.ko
 test -e /lib/modules/$KERNELVER/extra/i2s.ko && insmod /lib/modules/$KERNELVER/extra/i2s.ko

# presecure fullsecure modules
 test -e /lib/modules/$KERNELVER/extra/otp.ko && insmod /lib/modules/$KERNELVER/extra/otp.ko

# EPON Module
test -e /lib/modules/$KERNELVER/extra/epon_stack.ko && insmod /lib/modules/$KERNELVER/extra/epon_stack.ko epon_usr_init=1

# RDPA Command Drivers
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_tm.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_tm.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_iptv.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_iptv.ko 
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_ic.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_ic.ko 
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_sys.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_sys.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_br.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_br.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_port.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_port.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_llid.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_llid.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_spdsvc.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_spdsvc.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_ds_wan_udp_filter.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_ds_wan_udp_filter.ko
 test -e /lib/modules/$KERNELVER/extra/rdpa.ko && test -e /lib/modules/$KERNELVER/extra/rdpa_cmd_drv.ko && insmod /lib/modules/$KERNELVER/extra/rdpa_cmd_drv.ko 

# watchdog
 test -e /lib/modules/$KERNELVER/extra/bcm963138wdt.ko && insmod /lib/modules/$KERNELVER/extra/bcm963138wdt.ko

 # Actiontec add Sensor driver
test -e /lib/modules/$KERNELVER/kernel/drivers/misc/lis3lv02d/lis3lv02d.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/misc/lis3lv02d/lis3lv02d.ko 
test -e /lib/modules/$KERNELVER/kernel/drivers/misc/lis3lv02d/lis3lv02d_spi.ko && insmod /lib/modules/$KERNELVER/kernel/drivers/misc/lis3lv02d/lis3lv02d_spi.ko 
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


