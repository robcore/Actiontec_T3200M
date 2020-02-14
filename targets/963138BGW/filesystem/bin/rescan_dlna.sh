#!/bin/sh
cd /var/
if [ 0 -gt $1 ] || [ 1 -gt $2 ]; then
    echo "Invalid input params to rescan dlna!"
else
    while [ $1 -gt 0 ]
    do
        sleep $1
        kill -10 $2 > /dev/null 2>&1
    done
fi
