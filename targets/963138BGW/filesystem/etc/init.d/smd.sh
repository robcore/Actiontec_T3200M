#!/bin/sh

CRASHLOG_DIR=/data/crash_log

case "$1" in
	start)
		echo "Starting CMS smd..."
		if [ -e /data/aei_consolg_enable ]; then
			insmod /bin/aei_console.ko
		fi
		if [ ! -d ${CRASHLOG_DIR} ]; then
			mkdir ${CRASHLOG_DIR}
		fi
		str=`ls /data/crashlog* 2>/dev/null`
		if [ -n "${str}" ]; then
			cp /data/crashlog* ${CRASHLOG_DIR}
			rm -f /data/crashlog*
		fi
		/bin/smd
		/bin/mp_burnin.sh &
		exit 0
		;;

	stop)
		echo "Stopping CMS smd..."
		/bin/send_cms_msg -r 0x1000080D 20
		exit 0
		;;

	*)
		echo "$0: unrecognized option $1"
		exit 1
		;;

esac

