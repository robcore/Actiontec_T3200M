#! /bin/sh

# unit in minutes
# 2(minute) * 60 = 120s
WAIT_SYSTEM_UP_TIME=120
# 2880 = 3*60 + 45*60
BURN_IN_TEST_TIME=2880

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

burn_in_succ()
{
    echo "Burn-in Test: PASS"
    led_green_on
    cfecmd -s BurnInStatus 1

    return 0
}

burn_in_fail ()
{
    echo "Burn-in Test: FAIL"
    led_off
    led_red_on

    exit 1
}

burn_in_init ()
{
    # 1. Check Manu
    /bin/mp_is_manu_mode.sh
    ManuMode=$?
    if [ "$ManuMode" != "1" ] ; then
        # "Not in manufactory mode"
        echo "Not in manufactory mode, skip Burn-In Test."
        exit 1
    fi

    # 2. Waiting for DUT bringup,
    #    at least 110 seconds since DUT poweron.
    UpTime=$(read btime ftime < /proc/uptime;echo ${btime%.*})
    while [ $UpTime -lt $WAIT_SYSTEM_UP_TIME ]
    do
        sleep 5
        UpTime=$(read btime ftime < /proc/uptime;echo ${btime%.*})
    done

    # 3. Manufactory mode Initial, ONLY run once since DUT bringup
    MANU_INIT_FLAG="/tmp/mp_manu_inited.lock"
    if [ ! -f "$MANU_INIT_FLAG" ] ; then
        echo "Manufactory mode: Initial"
        # 3.1. Enable telnet
        # currently telnet is enabled by default, you can startup it when it become disabled in feature.
        /bin/start_telnetd.sh

        # 3.1.1 Enable ICMP. After 3 continus success PING response,
        #       test utility on PC will try to telnet to DUT.
        iptables -I INPUT -p icmp --icmp-type echo-request -j ACCEPT
        # 3.2. Disable WPS
        #cli -s InternetGatewayDevice.LANDevice.1.WLANConfiguration.1.WPS.Enable boolean 0
        #cli -s InternetGatewayDevice.LANDevice.1.WLANConfiguration.2.WPS.Enable boolean 0

        # 3.3 Set Manufactory mode init flag
        echo 1 > "$MANU_INIT_FLAG"
    fi

    # 4. Check Burn-In Status
    BurnInStatus=$(cfecmd -g BurnInStatus | grep "0")
    if [ -z "${BurnInStatus}" ] ; then
        BurnInStatus=1
    else
        BurnInStatus=0
    fi

    if [ "$BurnInStatus" == "1" ] ; then
        # "No need to do Burn-in Test"
        echo "No need to do Burn-in Test"
        exit 1
    fi

    # 5. Burn-In Test Initial
    echo "Burn-in Test: Initial"
    # 5.1. Off LEDs
    led_off

    return 0
}


burn_in_main()
{
    echo "Burn-in Test: Start"

    TestTime=$(read btime ftime < /proc/uptime;echo ${btime%.*})
    while [ $TestTime -lt $BURN_IN_TEST_TIME ]
    do
        /bin/mp_check_boot_status.sh
        CheckBoot=$?
        if [ $CheckBoot -eq 1 ] ; then
            return 1
        fi

        sec=0
        while [ $sec -lt 30 ]
        do
            led_green_on
            sleep 1

            led_off
            sleep 1

            let "sec+=1"
        done
        TestTime=$(read btime ftime < /proc/uptime;echo ${btime%.*})
    done

    return 0
}


burn_in_init
burn_in_main
if [ $? -eq 0 ] ; then
    burn_in_succ
else
    burn_in_fail
fi

