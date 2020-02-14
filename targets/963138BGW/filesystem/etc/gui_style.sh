#!/bin/sh

# add by Actiontec
CSS_PATH="/data/webs"
resetguistyle=0

for filename in `ls webs/style`
do
    if [ ! -e $CSS_PATH/$filename ]; then
        echo "########file $CSS_PATH/$filename does not exist"
        resetguistyle=1
        break
    fi
done

if [ "$resetguistyle" == "1" ];then
    echo "restore factory default gui style..."
    if [ -d $CSS_PATH ]; then
        rm -rf $CSS_PATH
    fi
    cp -rf /webs/style /data/webs 2>/dev/null
fi

