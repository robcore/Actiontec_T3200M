#! /bin/sh

SUCC=0
FAIL=1
SUCC1=2

. /bin/mp_configure

check_init ()
{
    #echo "Check USB Test: Initial ..."
    #mount usbfs
    mount -t usbfs none /proc/bus/usb  > /dev/null 2>&1

    return 0
}

check_usb()
{
    PORT=$1
    port_rst=$(grep "Prnt=01 Port=0${PORT}" /proc/bus/usb/devices -c )
    type_2_rst=$(grep "Prnt=01 Port=0${PORT}.*Spd=480" /proc/bus/usb/devices -c )
    type_3_rst=$(grep "Prnt=01 Port=0${PORT}.*Spd=5000" /proc/bus/usb/devices -c )
    
    PORT=$((PORT+1))
    if [ 1 -eq ${type_2_rst} ] ; then
        echo "USB${PORT} 2.0 Test:      PASS"
    elif [ 1 -eq ${type_3_rst} ] ; then
        echo "USB${PORT} 3.0 Test:      PASS"
    else 
        echo "USB${PORT} Test:      FAIL"
        return $FAIL;
    fi

    return $SUCC;
}

check_main()
{
    CheckStatus=$SUCC
    Result=$SUCC

    echo "Check USB Detect:"
    echo ""

    # 1. Check USB port1
    check_usb 0
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        Result=$FAIL
    fi
    #For T3200M, check only USB port 1
    if [ $SINGLE_USB == 1 ];then
        return $Result;
    fi

    # 2. Check USB port2
    check_usb 1
    CheckStatus=$?
    if [ $CheckStatus -eq $FAIL ] ; then
        Result=$FAIL
    fi
    
    return $Result
}
led_green_on ()
{
    # POWER LED
    mp_led.sh on POWER

    # WPS LED
    mp_led.sh on WPS
}

led_red_on ()
{
    # POWER LED
    mp_led.sh onnext POWER
    # WPS LED
    mp_led.sh onnext WPS
}

led_off ()
{
    # POWER LED
    mp_led.sh off POWER
    # WPS LED
    mp_led.sh off WPS
}

check_usb_succ()
{
    led_off
    led_green_on

    return 0
}

check_usb_fail ()
{
    led_off
    led_red_on

    exit 1
}

check_init
check_main
checkStatus=$?
if [ "-$1" = "-on" ] ; then
    if [ $checkStatus -eq 0 ] ; then
        check_usb_succ
    else
        check_usb_fail
    fi
fi
exit 0
