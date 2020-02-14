#! /bin/sh

SUCC=0
FAIL=1
SKIP=2

SKIP_5G=0
SKIP_SFP_WAN=0
SKIP_3D=0

if [ "$1" != "" ] ; then
    if [ "$1" == "--skip5g" ] ; then
        SKIP_5G=1
    fi
fi

. /bin/mp_configure

check_init ()
{
    #echo "Check Boot Test: Initial ..."

    result=$(qcsapi_sockrpc get_bootcfg_param calstate | grep "1")
    if [ -n "${result}" ] ; then
        echo "calstate=1, need do calibration! force skip 5G"
        SKIP_5G=1
    fi

    return 0
}

check_mem()
{
    #if set TM Memory 20--->44, DHD 0 Memory 0--->14, DHD 1 Memory 0--->7, 
    #kernel will take up more memory, user space memory will correspondingly smaller(18.... kB)
    result=$(cat /proc/meminfo | grep "MemTotal" | grep -e "22.... kB" -e "18.... kB")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_fs()
{
    result=$(mount | grep "mtd:data on /data type jffs2 (rw,relatime)")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_eth_lan()
{
    ETH="eth0 eth1 eth2 eth3"
    for eth in $ETH ; do
    	result=$(ifconfig $eth | grep "UP")
    	if [ -z "${result}" ] ; then
    	    result=$FAIL;
    	    return $result
    	else
    	    result=$SUCC;
    	fi
    done

    return $result
}

check_eth_wan()
{
    result=$(ifconfig ewan0 | grep "UP")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_sfp_wan()
{
    if [ $SKIP_SFP_WAN == 1 ] ;  then
        return $SKIP
    fi
    result=$(ifconfig ewan1 | grep "UP")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_wlan_2g()
{
    result=$(wl isup | grep "1")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_wlan_5g()
{
    if [ $SKIP_5G == 1 ] ;  then
        return $SKIP
    fi
    result=$(qcsapi_sockrpc get_status wifi0 | grep "Up")
    if [ -z "${result}" ] ; then
        result=$FAIL;
        return $result
    else
        result=$SUCC;
    fi

    return $result
}


check_moca()
{

    result=$(mocap moca$1 get --link | grep "link_status")
    if [ -z "${result}" ] ; then
        result=$FAIL;
    else
        result=$SUCC;
    fi
    return $result
}

check_3D()
{
    if [ $SKIP_3D == 1 ] ;  then
        return $SKIP
    fi
    evtest /dev/input/event0 2> /dev/null > /tmp/event_log &
    sleep 1
    killall -9 evtest 2> /dev/null

    result=$(cat /tmp/event_log | grep "Event: time")
    if [ -z "${result}" ] ; then
        result=$FAIL;
        return $result
    else
        result=$SUCC;
    fi

    return $result
}

check_daughter_card()
{
    if [ $SUPPORT_PCA9555 == 1 ]; then
        result=$(cat /proc/pca9555/output | grep "INPUT = 0")
    else
        #T3200M NO PCA9555, use GPIO_13 to check whether daughter card exist or not
        result=0x$(t=`dw fffe8114` && echo ${t:14})
        result=$(echo $((result & 0x2000)) | grep "8192")
    fi

    if [ -z "${result}" ] ; then
        result=$SUCC;
    else
        result=$FAIL;
    fi

    return $result
}

check_main()
{
    CheckStatus=$SUCC
    Result=$SUCC

    echo "Check Boot Test:"
    echo ""

    # 1. Check Memory Size
    check_mem
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "Mem Size Test:      FAIL"
        Result=$FAIL
    else
        echo "Mem Size Test:      PASS"
    fi

    # 2. Check File System
    check_fs
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "File System Test:   FAIL"
        Result=$FAIL
    else
        echo "File System Test:   PASS"
    fi

    # 3. Check Ethernet LAN
    check_eth_lan
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "Ethernet LAN Test:      FAIL"
        Result=$FAIL
    else
        echo "Ethernet LAN Test:      PASS"
    fi

    # 4. Check Ethernet WAN
    check_eth_wan
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "Ethernet WAN Test:      FAIL"
        Result=$FAIL
    else
        echo "Ethernet WAN Test:      PASS"
    fi

    # 5. Check SFP WAN
    if [ $SUPPORT_SFP_WAN == 1 ] ;  then
        check_sfp_wan
        CheckStatus=$?
        if [ $CheckStatus -eq $FAIL ] ; then
            echo "SFP WAN Test:      FAIL"
            Result=$FAIL
        elif [ $CheckStatus -eq $SKIP ] ; then
            echo "SFP WAN Test:      SKIP"
        else
            echo "SFP WAN Test:      PASS"
        fi
    fi

    # 6. Check wlan 2G
    check_wlan_2g
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "Wlan 2G Test:       FAIL"
        Result=$FAIL
    else
        echo "Wlan 2G Test:       PASS"
    fi

    # 7. Check wlan 5G
    check_wlan_5g
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        echo "Wlan 5G Test:       FAIL"
        Result=$FAIL
    elif [ $CheckStatus -eq $SKIP ] ; then
        echo "Wlan 5G Test:       SKIP"
    else
        echo "Wlan 5G Test:       PASS"
    fi

    # 8. Check MoCA WAN
    if [ $SUPPORT_MOCA_WAN == 1 ] ;  then
        check_moca 0
        CheckStatus=$?
        if [ $CheckStatus -eq $FAIL ] ; then
            echo "MoCA WAN Test:          FAIL"
            Result=$FAIL
        else
            echo "MoCA WAN Test:          PASS"
        fi
    fi

    # 9. Check MoCA LAN
    if [ $SUPPORT_MOCA_LAN == 1 ]; then
        if [ $SUPPORT_MOCA_WAN == 0 ] ;  then
            check_moca 0
        else
            check_moca 1
        fi
        CheckStatus=$?
        if [ $CheckStatus -eq $FAIL ] ; then
            echo "MoCA LAN Test:          FAIL"
            Result=$FAIL
        else
            echo "MoCA LAN Test:          PASS"
        fi
    fi

    # 10. Check 3D Accelerometer
    check_3D
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        if [ $SUPPORT_3D == 1 ]; then
            echo "3D Accelerometer Test:       FAIL"
            Result=$FAIL
        else
            echo "3D Accelerometer Test:       Absence"
        fi
    elif [ $CheckStatus -eq $SKIP ] ; then
        echo "3D Accelerometer Test:       SKIP"
    else
        if [ $SUPPORT_3D == 1 ]; then
            echo "3D Accelerometer Test:       PASS"
        else
            echo "3D Accelerometer Test:       Presence"
        fi
    fi

    # 11. Check daughter card
    if [ $SUPPORT_DAUTHER_CARD == 1 ]; then
        check_daughter_card
        CheckStatus=$?
        if [ $CheckStatus -eq $FAIL ] ; then
            echo "Daughter card Test:       Absence"
#            echo "Daughter card Test:       FAIL"
#            Result=$FAIL
        else
            echo "Daughter card Test:       Presence"
#            echo "Daughter card Test:       PASS"
        fi
    fi

    return $Result
}

check_init
check_main

exit $?

