echo $1

for dev in /dev/ttyp*; do
	echo $1 2>/dev/null >$dev
done
