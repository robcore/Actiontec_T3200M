ManuStatus=$(cfecmd -g ManuStatus | grep "1")

if [ -z "${ManuStatus}" ] ; then
	exit 0
else
	exit 1
fi
