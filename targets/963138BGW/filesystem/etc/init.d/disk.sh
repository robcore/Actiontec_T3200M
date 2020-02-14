#!/bin/sh
mod='xhci-hcd ohci-hcd ehci-hcd bcm63xx_usb usb-storage ahci_platform bcm63xx_sata '

case $1 in
suspend)
	grep ^/dev/sd /proc/mounts | while read d r t o; do
		umount $r || umount -f $r
	done
	while read h s c c i i l l; do
		case $h in Host:)
		echo scsi remove-single-device ${s#scsi} $c $i $l > /proc/scsi/scsi ;;
		esac
	done < /proc/scsi/scsi
	for m in $mod; do rmmod -w $m; done
	;;
resume)
	grep -e${mod// /.ko -e}KERNELVER= /etc/init.d/bcm-base-drivers.sh |sh
	;;
esac
