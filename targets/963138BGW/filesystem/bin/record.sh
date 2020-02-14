#! /bin/sh

if [ $# -eq 0 ];then
    exit
else
file=$1;
if [ ! -f $file ]; then
        echo 1 > $file
        sync
else
        v=$(cat $file)
        v=`expr $v + 1`
        echo $v > $file
        sync
fi
fi
