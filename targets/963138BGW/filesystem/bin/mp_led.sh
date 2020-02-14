#! /bin/sh
#######################  help ###########################
# /bin/mp_led.sh CMD <LED>
# CMD = on | onnext | off
# LED = [SWLED] WIFI POWER WPS SFP ETH0 ETH1 ETH2 ETH3 WAN MOCA0 MOCA1
# SWLED = WIFI POWER WPS SFP

# Notes:
# 1) <LED> is an option, if not specified LED after CMD, means run command on all LEDs including sw led and hw led.
#    when SWLED is specified, only to control "WIFI POWER WPS SFP " sw led
#
#
OFF="00"
ON="01"
ONNEXT="02"
ALL_LEDS="WIFI POWER WPS ETH0 ETH1 ETH2 ETH3 WAN"
SW_LEDS="WIFI POWER WPS"
ONNEXT_LEDS="POWER WPS"

. /bin/mp_configure

if [ $SUPPORT_SFP_WAN == 1 ]; then
    if [ $SINGLE_SFP_LED == 1 ]; then
        ALL_LEDS=${ALL_LEDS}" SFP0"
        SW_LEDS=${SW_LEDS}" SFP0"
    else
        ALL_LEDS=${ALL_LEDS}" SFP0 SFP1"
        SW_LEDS=${SW_LEDS}" SFP0 SFP1"
    fi
fi

if [ $SUPPORT_DSL_WAN == 1 ]; then
    ALL_LEDS=${ALL_LEDS}" VDSL0 VDSL1"
fi

if [[ $SUPPORT_MOCA_WAN == 1 && $SUPPORT_MOCA_LAN == 1 ]]; then
    ALL_LEDS=${ALL_LEDS}" MOCA0 MOCA1"
elif [[ $SUPPORT_MOCA_WAN == 1 || $SUPPORT_MOCA_LAN == 1 ]]; then
    ALL_LEDS=${ALL_LEDS}" MOCA0"
fi

if [ $SUPPORT_VOIP == 1 ]; then
    ALL_LEDS=${ALL_LEDS}" VOIP0 VOIP1"
fi

LEDS=$ALL_LEDS

if [ "$#" = "0" ] || [ "$1" = "help" ]; then
  echo "---HELP---"
  echo "$0 on/off (on/off all LED)"
  echo "$0 on/off SWLED (on/off SWLED: ${SW_LEDS})"
  echo "$0 on/off LED_NAME (on/off LED: ${ALL_LEDS})"
  echo "$0 onnext (on all red LED: ${ONNEXT_LEDS})"
  echo "$0 onnext LED_NAME (on red LED: ${ONNEXT_LEDS})"
  echo "---END---"
  exit 1;
fi

#/proc/led index
if [ $SPECAL_LED_INDEX == 1 ]; then
  POWER_LED_INDEX="22"
  SFP_LED0_INDEX="1F"
  SFP_LED1_INDEX="20"
else
  POWER_LED_INDEX="1E"
  SFP_LED0_INDEX="1B"
  SFP_LED1_INDEX="1C"
fi
WPS_LED_INDEX="03"
WIFI_LED_INDEX="1D"
ETH_LAN0_LED0_INDEX="0C"
ETH_LAN0_LED1_INDEX="0D"
ETH_LAN1_LED0_INDEX="0F"
ETH_LAN1_LED1_INDEX="10"
ETH_LAN2_LED0_INDEX="12"
ETH_LAN2_LED1_INDEX="13"
ETH_LAN3_LED0_INDEX="15"
ETH_LAN3_LED1_INDEX="16"
DSL0_LED_INDEX="00"
DSL1_LED_INDEX="01"
VOIP0_LED_INDEX="05"
VOIP1_LED_INDEX="06"

set_power_led()
{
    echo "${POWER_LED_INDEX}$1" >/proc/led
}

set_wps_led()
{
    echo "${WPS_LED_INDEX}$1" >/proc/led
}

set_wifi_led()
{
#    # after wifi-led supported, need to comment command#1 and uncomment  below command#2
#    # command#1 to control wifi-led
#    # echo "1C$1" >/proc/led
#
#    # command#2 to control wifi-led
#    case $1 in
#    "00")
#        # 2G
#        wl down
#        wl radio off
#        # 5G
#        qcsapi_sockrpc enable_interface wifi0 0
#        qcsapi_sockrpc set_LED 1 0
#        ;;
#    "01")
#        # 2G
#        wl radio on
#        wl up
#        # 5G
#        qcsapi_sockrpc enable_interface wifi0 1
#        qcsapi_sockrpc set_LED 1 1
#        ;;
#    esac

    # command#2 to control wifi-led
     echo "${WIFI_LED_INDEX}$1" >/proc/led
}

set_sfp_led0()
{
    echo "${SFP_LED0_INDEX}$1" >/proc/led
}

set_sfp_led1()
{
    echo "${SFP_LED1_INDEX}$1" >/proc/led
}

set_lan0_led()
{
    echo "${ETH_LAN0_LED0_INDEX}$1" >/proc/led
    echo "${ETH_LAN0_LED1_INDEX}$1" >/proc/led
}

set_lan1_led()
{
    echo "${ETH_LAN1_LED0_INDEX}$1" >/proc/led
    echo "${ETH_LAN1_LED1_INDEX}$1" >/proc/led
}

set_lan2_led()
{
    echo "${ETH_LAN2_LED0_INDEX}$1" >/proc/led
    echo "${ETH_LAN2_LED1_INDEX}$1" >/proc/led
}

set_lan3_led()
{
    echo "${ETH_LAN3_LED0_INDEX}$1" >/proc/led
    echo "${ETH_LAN3_LED1_INDEX}$1" >/proc/led
}

set_wan_led()
{
    #echo "19$1" >/proc/led
    #echo "1A$1" >/proc/led

    case $1 in
    "00")
        sw fffe8140 0x5015;sw fffe8144  21;sw fffe8140 0x5014;sw fffe8144  21;sw fffe8100 300000;sw fffe8114 300000
        sw fffe8140 0x4015;sw fffe8144  21;sw fffe8140 0x4014;sw fffe8144  21;
        ;;
    "01")
        sw fffe8140 0x5015;sw fffe8144  21;sw fffe8140 0x5014;sw fffe8144  21;sw fffe8100 300000;sw fffe8114 000000 
        ;;
    esac
}

set_moca_led()
{
    mode="00 01 02"
    for each in $mode; do
        if [ "-$2" = "-$each" ];then
            mocap $1 -p set --led_mode $2 --restart
            if [[ $SUPPORT_MOCA_WAN == 1 && $SUPPORT_MOCA_LAN == 1 && $1 = "MOCA0" ]]; then
                sleep 1
            fi
        fi
    done
}

set_vdsl0_led()
{
    echo "${DSL0_LED_INDEX}$1" >/proc/led
}

set_vdsl1_led()
{
    echo "${DSL1_LED_INDEX}$1" >/proc/led
}

set_voip0_led()
{
    if [ $SUPPORT_PCA9555 == 1 ]; then
        echo "08$1" >/proc/pca9555/output
    else
        echo "${VOIP0_LED_INDEX}$1" >/proc/led
    fi
}

set_voip1_led()
{
    if [ $SUPPORT_PCA9555 == 1 ]; then
        echo "09$1" >/proc/pca9555/output
    else
        echo "${VOIP1_LED_INDEX}$1" >/proc/led
    fi
}

set_led()
{
    case "$1" in
        "WIFI")
            set_wifi_led $2 ;;
        "POWER")
            set_power_led $2 ;;
        "WPS")
            set_wps_led $2 ;;
        "SFP0")
            set_sfp_led0 $2 ;;
        "SFP1")
            set_sfp_led1 $2 ;;
        "ETH0")
            set_lan0_led $2 ;;
        "ETH1")
            set_lan1_led $2 ;;
        "ETH2")
            set_lan2_led $2 ;;
        "ETH3")
            set_lan3_led $2 ;;
        "WAN")
            set_wan_led $2 ;;
        "MOCA0")
            set_moca_led moca0 $2 ;;
        "MOCA1")
            set_moca_led moca1 $2 ;;
        "VDSL0")
            set_vdsl0_led $2 ;;
        "VDSL1")
            set_vdsl1_led $2 ;;
        "VOIP0")
            set_voip0_led $2 ;;
        "VOIP1")
            set_voip1_led $2 ;;
        "SW")
            ;;
        *)
            echo "error $2 $1";;
    esac
}

set_led_off()
{
    set_led $1 $OFF
}

set_led_on()
{
    set_led $1 $ON
}

set_led_onnext()
{
    set_led $1 $ONNEXT
}


set_all_leds_off()
{
    for each in $LEDS ;do
        set_led_off $each
    done
}

set_all_leds_on()
{
    for each in $LEDS ;do
        set_led_on $each
    done
}

set_all_leds_onnext()
{
    for each in $ONNEXT_LEDS ;do
        set_led_onnext $each
    done
}


if [ $# -eq 0 ]; then
exit
else
    if [ "-$2" = "-SWLED" ]; then
        LEDS=$SW_LEDS
    fi

    case "$1" in
    "on")
        if [ "-$2" = "-" -o "-$2" = "-SWLED" ];then
                # turn on all LED
            set_all_leds_on
        else
            # turn led on
            set_led_on $2
        fi;;

    "off")
        if [ "-$2" = "-" -o "-$2" = "-SWLED" ];then
            # turn on all LED
            set_all_leds_off
        else
            # turn led off
            set_led_off $2
        fi ;;
    "onnext")
        if [ "-$2" = "-" -o "-$2" = "-SWLED" ];then
            # turn on all red led
            set_all_leds_onnext
        else
            # turn red led on
            set_led_onnext $2
        fi;;
    *)
        echo "error $1";;
    esac
fi
