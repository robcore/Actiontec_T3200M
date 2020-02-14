#!/bin/sh
cd /var/putlog
[[ -z "$1" ]] && MaxNumFiles="50" || MaxNumFiles="$1"
IsRunning=$(ps -a | grep -v grep | grep aei_deleteOldestProfileLog | grep -c ^)
if [ $IsRunning -gt 2 ]; then
    exit
fi
while [ 1 -gt 0 ]
do
    numfiles=$(ls -tr profileLog* 2>/dev/null | grep -v ^d  | grep -c ^)
    while [ $numfiles -gt $MaxNumFiles ]
    do
        echo "Delelte the oldest profile log..."
        oldestfile=$(ls -tr profileLog* | grep -v ^d  | grep -m1 "")
    	rm -f $oldestfile
        numfiles=$(( $numfiles -1 ))
    done
    sleep 60
done
