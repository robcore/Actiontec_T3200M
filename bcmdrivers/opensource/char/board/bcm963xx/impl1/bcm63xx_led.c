/*
    Copyright 2000-2011 Broadcom Corporation

    <:label-BRCM:2011:DUAL/GPL:standard
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation (the "GPL").
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    
    A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
    writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    
    :>
*/
/***************************************************************************
 * File Name  : bcm63xx_led.c
 *
 * Description: 
 *    This file contains bcm963xx board led control API functions. 
 *
 ***************************************************************************/

/* Includes. */
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/bcm_assert_locks.h>
#include <asm/uaccess.h>

#include <bcm_map_part.h>
#include <board.h>
#include <boardparms.h>
#include <shared_utils.h>
#include <bcm_led.h>
#include <bcmtypes.h>

extern spinlock_t bcm_gpio_spinlock;

#if defined(GPL_CODE_FACTORY_TEST)
extern unsigned int isInManuMode(void);
#endif

#define k125ms              (HZ / 8)   // 125 ms
#define kFastBlinkCount     0          // 125ms
#define kSlowBlinkCount     1          // 250ms
#define kVerySlowBlinkCount     3          // 500ms
#if defined(GPL_CODE) || defined(GPL_CODE)
#define kOneSecondBlinkCount 7          //1 second
#endif
#define kLedOff             0
#define kLedOn              1

#define kLedGreen           0
#define kLedRed             1

// uncomment // for debug led
// #define DEBUG_LED

typedef int (*BP_LED_FUNC) (unsigned short *);

typedef struct {
    BOARD_LED_NAME ledName;
    BP_LED_FUNC bpFunc;
    BP_LED_FUNC bpFuncFail;
} BP_LED_INFO, *PBP_LED_INFO;

typedef struct {
    short ledGreenGpio;             // GPIO # for LED
    short ledRedGpio;               // GPIO # for Fail LED
    BOARD_LED_STATE ledState;       // current led state
    short blinkCountDown;           // Count for blink states
#if defined(GPL_CODE)
    short blinkLedState;           //current blink led state: on/off
#endif
} LED_CTRL, *PLED_CTRL;

static BP_LED_INFO bpLedInfo[] =
{
    {kLedAdsl, BpGetAdslLedGpio, BpGetAdslFailLedGpio},
    {kLedSecAdsl, BpGetSecAdslLedGpio, BpGetSecAdslFailLedGpio},
    {kLedWanData, BpGetWanDataLedGpio, BpGetWanErrorLedGpio},
#if defined(GPL_CODE)
    {kLedSes, BpGetWirelessSesLedGpio, BpGetWirelessFailSesLedGpio},
#else
    {kLedSes, BpGetWirelessSesLedGpio, NULL},
#endif
    {kLedVoip, BpGetVoipLedGpio, NULL},
    {kLedVoip1, BpGetVoip1LedGpio, BpGetVoip1FailLedGpio},
    {kLedVoip2, BpGetVoip2LedGpio, BpGetVoip2FailLedGpio},
    {kLedPots, BpGetPotsLedGpio, NULL},
    {kLedDect, BpGetDectLedGpio, NULL},
    {kLedGpon, BpGetGponLedGpio, BpGetGponFailLedGpio},
    {kLedMoCA, BpGetMoCALedGpio, BpGetMoCAFailLedGpio},
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    {kLedOpticalLink,  NULL, BpGetOpticalLinkFailLedGpio},
    {kLedUSB, BpGetUSBLedGpio, NULL},
    {kLedSim, BpGetGpioLedSim, NULL},
    {kLedSimITMS, BpGetGpioLedSimITMS, NULL},
    {kLedEpon, BpGetEponLedGpio, BpGetEponFailLedGpio},
#endif
#if defined(CONFIG_BCM968500)
    {kLedOpticalLink,  NULL, BpGetOpticalLinkFailLedGpio},
    {kLedLan1, UtilGetLan1LedGpio, NULL},
    {kLedLan2, UtilGetLan2LedGpio, NULL},
    {kLedLan3, UtilGetLan3LedGpio, NULL},
    {kLedLan4, UtilGetLan4LedGpio, NULL},
    {kLedUSB, BpGetUSBLedGpio, NULL}, 
#endif
#if defined(GPL_CODE)
    {kLedUsb, BpGetUSBLedGpio, NULL},
    {kLedPower, BpGetBootloaderPowerOnLedGpio, BpGetBootloaderStopLedGpio},
    {kLedSes, BpGetWirelessSesLedGpio, BpGetWirelessFailSesLedGpio},
#if defined(GPL_CODE)
    {AEI_kLedWlanAct, BpGetWirelessLedGpioAct, NULL},
    {AEI_kLedWlan, BpGetWirelessLedGpioGreen, BpGetWirelessLedGpioRed},
    {AEI_kLedWlanGreen, BpGetWirelessLedGpioGreen, NULL},
    {AEI_kLedWlanRed, BpGetWirelessLedGpioRed, NULL},
#endif
#if defined(GPL_CODE_63168_CHIP)
    {kLedEnetWan, BpGetEnetWanLedGpio, NULL},
#endif
#if defined(GPL_CODE_63138_CHIP)
    {kLedWanEthLink, BpGetWanEthLinkLedGpio, NULL},
    {kLedWanEthSpeed100, BpGetWanEthSpeed100LedGpio, NULL},
    {kLedWanEthSpeed1000, BpGetWanEthSpeed1000LedGpio, NULL},
#endif
#if defined(GPL_CODE)
#if defined(GPL_CODE_63138_CHIP)
    {kLedWifi, BpGetWifiLedGpio, NULL},
    {kLedWanSfpLink0, BpGetWanSfpLinkLedGpio, NULL},
    {kLedWanSfpLink1, BpGetWanSfpLinkFailLedGpio, NULL},
    {kLedEth0Spd100, BpGetEth0Spd100LedGpio, NULL},
    {kLedEth0Spd1000, BpGetEth0Spd1000LedGpio, NULL},
    {kLedEth1Spd100, BpGetEth1Spd100LedGpio, NULL},
    {kLedEth1Spd1000, BpGetEth1Spd1000LedGpio, NULL},
    {kLedEth2Spd100, BpGetEth2Spd100LedGpio, NULL},
    {kLedEth2Spd1000, BpGetEth2Spd1000LedGpio, NULL},
    {kLedEth3Spd100, BpGetEth3Spd100LedGpio, NULL},
    {kLedEth3Spd1000, BpGetEth3Spd1000LedGpio, NULL},
#endif
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
    {kLedInternet, BpGetInternetGreenLedGpio , BpGetInternetRedLedGpio},
#endif
#endif
#endif
    {kLedEnd, NULL, NULL}
};

// global variables:
static struct timer_list gLedTimer;
static PLED_CTRL gLedCtrl = NULL;
static int gTimerOn = FALSE;
static int gTimerOnRequests = 0;
static unsigned int gLedRunningCounter = 0;  // only used by WLAN

#if defined(GPL_CODE) || defined(GPL_CODE)
static bool gPowerLedLocalUpgrade = FALSE;
static bool gPowerLedTr69Upgrade = TRUE;
int  gPowerLedStatus = 1;                              //1:green; 2:amber and green alternative;3 amber blink
static unsigned int gWANLedRunningCounter = 0;  // only used by WAN
#endif

#if defined(GPL_CODE)
#define WIFILED_TIMER_INTERVAL 10 /* milliseconds */
#define WIFILED_STATE_INTERVAL (1000 / WIFILED_TIMER_INTERVAL) /* intervals */
#if !defined(GPL_CODE)
static unsigned short wifi24g_gpio = BP_NOT_DEFINED;
static unsigned short wifi5g_gpio = BP_NOT_DEFINED;
static struct timer_list gwifiLedTimer;
#endif

void boardLedCtrl(BOARD_LED_NAME, BOARD_LED_STATE);
#endif

void (*ethsw_led_control)(unsigned long ledMask, int value) = NULL;
EXPORT_SYMBOL(ethsw_led_control);

/** This spinlock protects all access to gLedCtrl, gTimerOn
 *  gTimerOnRequests, gLedRunningCounter, and ledTimerStart function.
 *  Use spin_lock_irqsave to lock the spinlock because ledTimerStart
 *  may be called from interrupt handler (WLAN?)
 */
static spinlock_t brcm_ledlock;
static void ledTimerExpire(void);
#if defined(GPL_CODE)
void aei_set_internet_led_brightness(unsigned int brightness);
void aei_set_wifi_led_brightness(unsigned int brightness);
#endif

//**************************************************************************************
// LED operations
//**************************************************************************************
#if defined(GPL_CODE)
/* ken, Set HW control for WAN Data LED. */
void AEI_SetWanLedHwControl(BOARD_LED_NAME ledName,PLED_CTRL pLed,int enable)
{
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268)
    if (ledName == kLedWanData)
    {
        if(enable)
                LED->ledHWDis &= ~GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
        else
            LED->ledHWDis |= GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
    }
#elif defined(CONFIG_BCM96368)

    if (ledName == kLedWanData)
    {
        if(enable)
        {
                GPIO->AuxLedCtrl &= ~AUX_HW_DISAB_2;
                if (pLed->ledGreenGpio & BP_ACTIVE_LOW)
                    GPIO->AuxLedCtrl &= ~(LED_STEADY_ON << AUX_MODE_SHFT_2);
                else
                    GPIO->AuxLedCtrl |= (LED_STEADY_ON << AUX_MODE_SHFT_2);
        }
        else
        {
            GPIO->AuxLedCtrl |= AUX_HW_DISAB_2;
            if (pLed->ledGreenGpio & BP_ACTIVE_LOW)
                GPIO->AuxLedCtrl |= (LED_STEADY_ON << AUX_MODE_SHFT_2);
            else
                GPIO->AuxLedCtrl &= ~(LED_STEADY_ON << AUX_MODE_SHFT_2);
        }
    }

#endif

}
#endif
// turn led on and set the ledState
static void setLed (PLED_CTRL pLed, unsigned short led_state, unsigned short led_type)
{
    short led_gpio;
    unsigned short gpio_state;
    unsigned long flags;


    if (led_type == kLedRed)
        led_gpio = pLed->ledRedGpio;
    else
        led_gpio = pLed->ledGreenGpio;

        dev_dbg(NULL,  "********************************************************\n");
        dev_dbg(NULL,  "setLed %d %x\n", led_gpio&0xff, led_gpio);
        dev_dbg(NULL,  "********************************************************\n");

    if (led_gpio == -1)
        return;

    if (((led_gpio & BP_ACTIVE_LOW) && (led_state == kLedOn)) || 
        (!(led_gpio & BP_ACTIVE_LOW) && (led_state == kLedOff)))
        gpio_state = 0;
    else
        gpio_state = 1;

    /* spinlock to protect access to GPIODir, GPIOio */
    spin_lock_irqsave(&bcm_gpio_spinlock, flags);

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96318)
    /* Enable LED controller to drive this GPIO */
    if (!(led_gpio & BP_GPIO_SERIAL))
        GPIO->GPIOMode |= GPIO_NUM_TO_MASK(led_gpio);
#endif

#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) 
    /* Enable LED controller to drive this GPIO */
    if (!(led_gpio & BP_GPIO_SERIAL))
        GPIO->LEDCtrl |= GPIO_NUM_TO_MASK(led_gpio);
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM960333) || defined(CONFIG_BCM96848)
    bcm_led_driver_set(led_gpio, led_state);
#endif


#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)
#ifdef GPL_CODE_63168_CHIP
    if ((led_gpio & BP_GPIO_NUM_MASK) > 31)
    {
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        if( gpio_state )
        {
            GPIO->GPIOio |= GPIO_NUM_TO_MASK(led_gpio);
        }
        else
        {
            GPIO->GPIOio &= ~GPIO_NUM_TO_MASK(led_gpio); //off
        }
    }
    else
    {
        LED->ledMode &= ~(LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        if( gpio_state )
            LED->ledMode |= (LED_MODE_OFF << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        else
            LED->ledMode |= (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    }
#else
    LED->ledMode &= ~(LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    if( gpio_state )
        LED->ledMode |= (LED_MODE_OFF << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    else
        LED->ledMode |= (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));

#endif
#elif defined(CONFIG_BCM96838)
    if ( (led_gpio&BP_LED_PIN) || (led_gpio&BP_GPIO_SERIAL) )
    {
        LED->ledMode &= ~(LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        if( gpio_state )
            LED->ledMode |= (LED_MODE_OFF << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        else
            LED->ledMode |= (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    }
    else
    {
        led_gpio &= BP_GPIO_NUM_MASK;
        gpio_set_dir(led_gpio, 1);
        gpio_set_data(led_gpio, gpio_state);
    }

#elif !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) && !defined(CONFIG_BCM963381) && !defined(CONFIG_BCM960333) && !defined(CONFIG_BCM96848)
    if (led_gpio & BP_GPIO_SERIAL) {
        while (GPIO->SerialLedCtrl & SER_LED_BUSY);
        if( gpio_state )
            GPIO->SerialLed |= GPIO_NUM_TO_MASK(led_gpio);
        else
            GPIO->SerialLed &= ~GPIO_NUM_TO_MASK(led_gpio);
    }
    else {
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        if( gpio_state )
            GPIO->GPIOio |= GPIO_NUM_TO_MASK(led_gpio);
        else
            GPIO->GPIOio &= ~GPIO_NUM_TO_MASK(led_gpio);
    }
#endif

    spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);

}

#if defined(GPL_CODE) && defined(GPL_CODE_63168_CHIP)
static void wpsledToggle(PLED_CTRL pLed)
{
    short led_gpio;
    short i;
    static unsigned short state;

    if (state == 0)
        state = 1;
    else
        state = 0;

    for (i=0;i<2;i++)/*To amber color*/
    {
        if (i==0)
            led_gpio = pLed->ledGreenGpio;
        else
            led_gpio = pLed->ledRedGpio;

        if (led_gpio == -1)
            return;

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268)
    if((BP_GPIO_NUM_MASK & led_gpio)>=2 && (BP_GPIO_NUM_MASK & led_gpio)<=7)
        printk("####set NAND FLASH gpio incorrectly (%d),LEDCtl(%x)\n",(BP_GPIO_NUM_MASK & led_gpio),GPIO->LEDCtrl);

    if((BP_GPIO_NUM_MASK & led_gpio)>=24 && (BP_GPIO_NUM_MASK & led_gpio)<=31)
        printk("####set NAND FLASH incorrectly (%d),LEDCtl(%x)\n",(BP_GPIO_NUM_MASK & led_gpio),GPIO->LEDCtrl);

        if ((led_gpio & BP_GPIO_NUM_MASK) > 31)
        {
            GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
            GPIO->GPIOio ^= (GPIO_NUM_TO_MASK(led_gpio));
        }
        else
        {
            LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        }
#else

        if (led_gpio & BP_GPIO_SERIAL) {
            while (GPIO->SerialLedCtrl & SER_LED_BUSY);
            GPIO->SerialLed ^= GPIO_NUM_TO_MASK(led_gpio);
        }
        else {
            unsigned long flags;

            spin_lock_irqsave(&bcm_gpio_spinlock, flags);
            GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
            GPIO->GPIOio ^= GPIO_NUM_TO_MASK(led_gpio);
            /*printk("GPIO->GPIOio=%x\n", GPIO->GPIOio);*/

            spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
        }
#endif
    }
}
#endif
// toggle the LED
static void ledToggle(PLED_CTRL pLed)
{
    short led_gpio;
    short green_led_gpio , red_led_gpio;

   green_led_gpio = pLed->ledGreenGpio ;
   red_led_gpio = pLed->ledRedGpio ;

    if ((-1== green_led_gpio) && (-1== red_led_gpio))
        return;

    /* Currently all the chips don't support blinking of RED LED */
    if (-1== green_led_gpio)
        return;
  
   led_gpio = green_led_gpio ;

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)
 #ifdef GPL_CODE_63168_CHIP
    if ((led_gpio & BP_GPIO_NUM_MASK) > 31)
    {
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        GPIO->GPIOio ^= (GPIO_NUM_TO_MASK(led_gpio));
    }
    else
    {
        LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    }
#else
    LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
#endif

#elif defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848) 
    bcm_led_driver_toggle(led_gpio);

#elif defined(CONFIG_BCM96838)
    if ( (led_gpio&BP_LED_PIN) || (led_gpio&BP_GPIO_SERIAL) )
        LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    else
    {
        unsigned long flags;
		led_gpio &= BP_GPIO_NUM_MASK;
        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        gpio_set_data(led_gpio, 1^gpio_get_data(led_gpio));
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
    }
	
#elif defined(CONFIG_BCM960333)
    {
        unsigned long flags;
        led_gpio &= BP_GPIO_NUM_MASK;
        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        bcm_led_driver_toggle(led_gpio);
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
    }
#else
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) && !defined(CONFIG_BCM963381) && !defined(CONFIG_BCM96848)
    if (led_gpio & BP_GPIO_SERIAL) {
        while (GPIO->SerialLedCtrl & SER_LED_BUSY);
        GPIO->SerialLed ^= GPIO_NUM_TO_MASK(led_gpio);
    }
    else {
        unsigned long flags;

        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        GPIO->GPIOio ^= GPIO_NUM_TO_MASK(led_gpio);
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
    }

#endif /* !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM960333) && !defined(CONFIG_BCM963148) */
#endif
}   
#if defined(GPL_CODE)
#if !defined(GPL_CODE) && !defined(GPL_CODE_63168_CHIP)
static void wifiLedTimerExpire(void)
{
    unsigned int val_24g = 0, val_5g = 0;
    static unsigned int state_24g, state_5g;

    mod_timer(&gwifiLedTimer, jiffies + msecs_to_jiffies(WIFILED_TIMER_INTERVAL));

    val_24g = kerSysGetGpioValue(wifi24g_gpio);
    if ((wifi24g_gpio & BP_ACTIVE_MASK) == BP_ACTIVE_LOW)
	val_24g = val_24g ? 0 : 1;

    if (val_24g)
    {
	state_24g = WIFILED_STATE_INTERVAL;
    }
    else if (state_24g)
    {
	--state_24g;
	boardLedCtrl(kLedWifi, kLedStateOff);
	return;
    }

    val_5g = kerSysGetGpioValue(wifi5g_gpio);
    if ((wifi5g_gpio & BP_ACTIVE_MASK) == BP_ACTIVE_LOW)
	val_5g = val_5g ? 0 : 1;

    if (val_5g)
    {
	state_5g = WIFILED_STATE_INTERVAL;
    }
    else if (state_5g)
    {
	--state_5g;
	boardLedCtrl(kLedWifi, kLedStateOff);
	return;
    }

    boardLedCtrl(kLedWifi, (state_24g || state_5g) ? kLedStateOn : kLedStateOff);

}

static void wifiLedTimerStart(void)
{
    int start_timer = 0;
    unsigned short gpio = BP_NOT_DEFINED;

    if (BpGetWifi2_4GHzLedGpio(&wifi24g_gpio) == BP_SUCCESS)
    {
	kerSysSetGpioDirInput(wifi24g_gpio);
	start_timer = 1;
    }

    if (BpGetWifi5GHzLedGpio(&wifi5g_gpio) == BP_SUCCESS)
    {
	kerSysSetGpioDirInput(wifi5g_gpio);
	start_timer = 1;
    }

    if (BpGetWifiLedGpio(&gpio) != BP_SUCCESS)
	start_timer = 0;

    if (!start_timer)
	return;

    init_timer(&gwifiLedTimer);
    gwifiLedTimer.function = (void*)wifiLedTimerExpire;
    gwifiLedTimer.expires = jiffies + msecs_to_jiffies(WIFILED_TIMER_INTERVAL);
    add_timer (&gwifiLedTimer);
    printk("Started WIFI led timer (%d ms)\n", WIFILED_TIMER_INTERVAL);
}
#endif
#endif
/** Start the LED timer
 *
 * Must be called with brcm_ledlock held
 */
static void ledTimerStart(void)
{
#if defined(DEBUG_LED)
    printk("led: add_timer\n");
#endif

    BCM_ASSERT_HAS_SPINLOCK_C(&brcm_ledlock);

    init_timer(&gLedTimer);
    gLedTimer.function = (void*)ledTimerExpire;
    gLedTimer.expires = jiffies + msecs_to_jiffies(125);        // timer expires in ~125ms
    add_timer (&gLedTimer);
} 


// led timer expire kicks in about ~100ms and perform the led operation according to the ledState and
// restart the timer according to ledState
static void ledTimerExpire(void)
{
    int i;
    PLED_CTRL pCurLed;
    unsigned long flags;

    BCM_ASSERT_NOT_HAS_SPINLOCK_V(&brcm_ledlock);

    for (i = 0, pCurLed = gLedCtrl; i < kLedEnd; i++, pCurLed++)
    {
#if defined(DEBUG_LED)
        printk("led[%d]: Mask=0x%04x, State = %d, blcd=%d\n", i, pCurLed->ledGreenGpio, pCurLed->ledState, pCurLed->blinkCountDown);
#endif
        spin_lock_irqsave(&brcm_ledlock, flags);        // LEDs can be changed from ISR
        switch (pCurLed->ledState)
        {
        case kLedStateOn:
        case kLedStateOff:
        case kLedStateFail:
#if defined(GPL_CODE)
        case kLedStateAmber:
#endif
            pCurLed->blinkCountDown = 0;            // reset the blink count down
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

#if defined(GPL_CODE)
        case kLedStateVerySlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kVerySlowBlinkCount;
                ledToggle(pCurLed);
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif

        case kLedStateSlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kSlowBlinkCount;
                ledToggle(pCurLed);
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

        case kLedStateFastBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
                ledToggle(pCurLed);
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#if defined(GPL_CODE) && defined(GPL_CODE_63168_CHIP)
        case kLedStateAmberSlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kOneSecondBlinkCount;
            if (gPowerLedLocalUpgrade == FALSE)
                {
                    gPowerLedLocalUpgrade = TRUE;
                    setLed(pCurLed, kLedOn, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                }
                else
                {
                    gPowerLedLocalUpgrade = FALSE;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOff, kLedRed);
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

        case kLedStateAmberAndGreenSlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kOneSecondBlinkCount;
                if (gPowerLedTr69Upgrade == FALSE)
                {
                    gPowerLedTr69Upgrade = TRUE;
                    setLed(pCurLed, kLedOn, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                }
                else
                {
                    gPowerLedTr69Upgrade = FALSE;
                    setLed(pCurLed, kLedOff, kLedRed);
                }

            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#else
#if defined(GPL_CODE)
        case kLedStateAmberSlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kSlowBlinkCount;
                if (pCurLed->blinkLedState == kLedOff) {
                    pCurLed->blinkLedState = kLedOn;
                    setLed(pCurLed, kLedOn, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                } else {
                    pCurLed->blinkLedState = kLedOff;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOff, kLedRed);
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
        case kLedStateAmberFastBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
                if (pCurLed->blinkLedState == kLedOff) {
                    pCurLed->blinkLedState = kLedOn;
                    setLed(pCurLed, kLedOn, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                } else {
                    pCurLed->blinkLedState = kLedOff;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOff, kLedRed);
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif
#endif

        case kLedStateUserWpsInProgress:          /* 200ms on, 100ms off */
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = (((gLedRunningCounter++)&1)? kFastBlinkCount: kSlowBlinkCount);
#if defined(GPL_CODE) && defined(GPL_CODE_63168_CHIP)
                wpsledToggle(pCurLed);
#else
                ledToggle(pCurLed);
#endif
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;             
#if defined(GPL_CODE)
        case kLedStateAmberUserWpsInProgress:      /* 200ms on, 100ms off */
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = (((gLedRunningCounter++)&1)? kFastBlinkCount: kSlowBlinkCount);
                if (pCurLed->blinkLedState == kLedOff) {
                    pCurLed->blinkLedState = kLedOn;
                    setLed(pCurLed, kLedOn, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                } else {
                    pCurLed->blinkLedState = kLedOff;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOff, kLedRed);
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
        case kLedStateRedUserWpsInProgress:      /* 200ms on, 100ms off */
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = (((gLedRunningCounter++)&1)? kFastBlinkCount: kSlowBlinkCount);
                if (pCurLed->blinkLedState == kLedOff) {
                    pCurLed->blinkLedState = kLedOn;
                    setLed(pCurLed, kLedOn, kLedRed);
                } else {
                    pCurLed->blinkLedState = kLedOff;
                    setLed(pCurLed, kLedOff, kLedRed);
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif
        case kLedStateUserWpsError:               /* 100ms on, 100ms off */
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
#if defined(GPL_CODE) && defined(GPL_CODE_63168_CHIP)
                wpsledToggle(pCurLed);
#else
#if defined(GPL_CODE)
                pCurLed->blinkLedState = kLedOn;
                setLed(pCurLed, kLedOn, kLedRed);
#else
                ledToggle(pCurLed);
#endif
#endif
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;        

        case kLedStateUserWpsSessionOverLap:      /* 100ms on, 100ms off, 5 times, off for 500ms */        
            if (pCurLed->blinkCountDown-- == 0)
            {
                if(((++gLedRunningCounter)%10) == 0) {
                    pCurLed->blinkCountDown = 4;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    pCurLed->ledState = kLedStateUserWpsSessionOverLap;
                }
                else
                {
                    pCurLed->blinkCountDown = kFastBlinkCount;
#if defined(GPL_CODE) && defined(GPL_CODE_63168_CHIP)
                    wpsledToggle(pCurLed);
#else
                    ledToggle(pCurLed);
#endif
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;        
#if defined(GPL_CODE)
        case kLedStateAmberUserWpsSessionOverLap:  /* 100ms on, 100ms off, 5 times, off for 500ms */
            if (pCurLed->blinkCountDown-- == 0)
            {
                if(((++gLedRunningCounter)%10) == 0) {
                    pCurLed->blinkCountDown = 4;
                    pCurLed->blinkLedState = kLedOff;
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOff, kLedRed);
                    pCurLed->ledState = kLedStateAmberUserWpsSessionOverLap;
                }
                else
                {
                    pCurLed->blinkCountDown = kFastBlinkCount;
                    if (pCurLed->blinkLedState == kLedOff) {
                        pCurLed->blinkLedState = kLedOn;
                        setLed(pCurLed, kLedOn, kLedGreen);
                        setLed(pCurLed, kLedOn, kLedRed);
                    } else {
                        pCurLed->blinkLedState = kLedOff;
                        setLed(pCurLed, kLedOff, kLedGreen);
                        setLed(pCurLed, kLedOff, kLedRed);
                    }
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif
#if defined(GPL_CODE) || defined(GPL_CODE)
        case kLedStateUserWANGreenRedVerySlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kVerySlowBlinkCount;
                if(gWANLedRunningCounter == 0)
                {
                    AEI_SetWanLedHwControl(kLedWanData,pCurLed,1);
                   setLed(pCurLed, kLedOff, kLedRed);
                   setLed(pCurLed, kLedOn, kLedGreen);
                    gWANLedRunningCounter = 1;
                }
                else
                {
                    AEI_SetWanLedHwControl(kLedWanData,pCurLed,0);
                    setLed(pCurLed, kLedOff, kLedGreen);
                    setLed(pCurLed, kLedOn, kLedRed);
                    gWANLedRunningCounter = 0;
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);

            break;
        case kLedStatePowerOneSecondBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
               pCurLed->blinkCountDown = kOneSecondBlinkCount;
               ledToggle(pCurLed);
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif
        default:
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            printk("Invalid state = %d\n", pCurLed->ledState);
        }
    }

    // Restart the timer if any of our previous LED operations required
    // us to restart the timer, or if any other threads requested a timer
    // restart.  Note that throughout this function, gTimerOn == TRUE, so
    // any other thread which want to restart the timer would only
    // increment the gTimerOnRequests and not call ledTimerStart.
    spin_lock_irqsave(&brcm_ledlock, flags);
    if (gTimerOnRequests > 0)
    {
        ledTimerStart();
        gTimerOnRequests = 0;
    }
    else
    {
        gTimerOn = FALSE;
    }
    spin_unlock_irqrestore(&brcm_ledlock, flags);
}



#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
void aei_flash_led(unsigned short num, unsigned int enable);
void aei_set_led_brightness(unsigned short num, unsigned int brightness);
#endif

void __init boardLedInit(void)
{
    PBP_LED_INFO pInfo;
    unsigned short i;
    short gpio;

    spin_lock_init(&brcm_ledlock);

#if !(defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM960333) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848))
    /* Set blink rate for hardware LEDs. */
    GPIO->LEDCtrl &= ~LED_INTERVAL_SET_MASK;
    GPIO->LEDCtrl |= LED_INTERVAL_SET_80MS;
#else
#if defined(GPL_CODE_63168_CHIP)
    GPIO->GPIOCtrl |= 0x000FFFC0;
#endif
#endif


    gLedCtrl = (PLED_CTRL) kmalloc((kLedEnd * sizeof(LED_CTRL)), GFP_KERNEL);

    if( gLedCtrl == NULL ) {
        printk( "LED memory allocation error.\n" );
        return;
    }

    /* Initialize LED control */
    for (i = 0; i < kLedEnd; i++) {
        gLedCtrl[i].ledGreenGpio = -1;
        gLedCtrl[i].ledRedGpio = -1;
        gLedCtrl[i].ledState = kLedStateOff;
        gLedCtrl[i].blinkCountDown = 0;            // reset the blink count down
    }

    for( pInfo = bpLedInfo; pInfo->ledName != kLedEnd; pInfo++ ) {
        if( pInfo->bpFunc && (*pInfo->bpFunc) (&gpio) == BP_SUCCESS )
        {
            gLedCtrl[pInfo->ledName].ledGreenGpio = gpio;
        }
        if( pInfo->bpFuncFail && (*pInfo->bpFuncFail)(&gpio)==BP_SUCCESS )
        {
            gLedCtrl[pInfo->ledName].ledRedGpio = gpio;
        }

#if defined(GPL_CODE)
        if (pInfo->ledName == kLedPower)
        {
            continue;
        }
#endif
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
        if (kLedInternet == pInfo->ledName) {
            boardLedCtrl(kLedInternet, kLedStateFastBlinkContinues);
            aei_flash_led(gLedCtrl[pInfo->ledName].ledGreenGpio, 0);
        } else
#endif
        {
        setLed(&gLedCtrl[pInfo->ledName], kLedOff, kLedGreen);
        setLed(&gLedCtrl[pInfo->ledName], kLedOff, kLedRed);
        }
    }
#if defined(GPL_CODE)
#if !defined(GPL_CODE) && !defined(GPL_CODE_63168_CHIP)
#if defined(GPL_CODE_FACTORY_TEST)
    if (!isInManuMode())
        wifiLedTimerStart();
#else
    wifiLedTimerStart();
#endif
#endif
#endif
#if defined(DEBUG_LED)
    for (i=0; i < kLedEnd; i++)
        printk("initLed: led[%d]: Gpio=0x%04x, FailGpio=0x%04x\n", i, gLedCtrl[i].ledGreenGpio, gLedCtrl[i].ledRedGpio);
#endif
}

// led ctrl.  Maps the ledName to the corresponding ledInfoPtr and perform the led operation
void boardLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    unsigned long flags;
    PLED_CTRL pLed;

    BCM_ASSERT_NOT_HAS_SPINLOCK_V(&brcm_ledlock);

    spin_lock_irqsave(&brcm_ledlock, flags);     // LED can be changed from ISR

    if( (int) ledName < kLedEnd ) {

        pLed = &gLedCtrl[ledName];

        // If the state is kLedStateFail and there is not a failure LED defined
        // in the board parameters, change the state to kLedStateSlowBlinkContinues.
        if( ledState == kLedStateFail && pLed->ledRedGpio == -1 )
            ledState = kLedStateSlowBlinkContinues;

        // Save current LED state
        pLed->ledState = ledState;
#if defined(GPL_CODE)
        pLed->blinkLedState == kLedOff;
#endif

        // Start from LED Off and turn it on later as needed
        setLed (pLed, kLedOff, kLedGreen);
        setLed (pLed, kLedOff, kLedRed);
#if defined(GPL_CODE)
//        aei_set_led_brightness(pLed->ledRedGpio, 8); // restore the red led brightness, change it later as needed
#endif

        // Disable HW control for WAN Data LED. 
        // It will be enabled only if LED state is On

#if defined(GPL_CODE)
        AEI_SetWanLedHwControl(ledName,pLed,0);
#else
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM96838)
        if (ledName == kLedWanData)
            LED->ledHWDis |= GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
#endif

#endif
        switch (ledState) {
        case kLedStateOn:
            // Enable SAR to control INET LED

#if defined(GPL_CODE)
            AEI_SetWanLedHwControl(ledName,pLed,1);
#else
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM96838)
            if (ledName == kLedWanData)
                LED->ledHWDis &= ~GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
#endif
#endif
            setLed (pLed, kLedOn, kLedGreen);
            break;

        case kLedStateOff:
            break;

        case kLedStateFail:
            setLed (pLed, kLedOn, kLedRed);
            break;

#if defined(GPL_CODE)
        case kLedStateAmber:
//            aei_set_led_brightness(pLed->ledRedGpio, 1);
            setLed (pLed, kLedOn, kLedGreen);
            setLed (pLed, kLedOn, kLedRed);
            break;

        case kLedStateVerySlowBlinkContinues:
            pLed->blinkCountDown = kVerySlowBlinkCount;
            gTimerOnRequests++;
            break;

#endif

        case kLedStateSlowBlinkContinues:
            pLed->blinkCountDown = kSlowBlinkCount;
            gTimerOnRequests++;
            break;

        case kLedStateFastBlinkContinues:
            pLed->blinkCountDown = kFastBlinkCount;
            gTimerOnRequests++;
            break;

#if defined(GPL_CODE)
        case kLedStateAmberSlowBlinkContinues:
//            aei_set_led_brightness(pLed->ledRedGpio, 1);
			gPowerLedStatus = 3;
            pLed->blinkCountDown = kOneSecondBlinkCount;
            gTimerOnRequests++;
            break;

        case kLedStateAmberAndGreenSlowBlinkContinues:
//            aei_set_led_brightness(pLed->ledRedGpio, 1);
			gPowerLedStatus = 2;
            pLed->blinkCountDown = kOneSecondBlinkCount;
            gTimerOnRequests++;
            break;
#elif defined(GPL_CODE)
        case kLedStateAmberSlowBlinkContinues:
//            aei_set_led_brightness(pLed->ledRedGpio, 1);
            pLed->blinkCountDown = kSlowBlinkCount;
            gTimerOnRequests++;
            break;
        case kLedStateAmberFastBlinkContinues:
//            aei_set_led_brightness(pLed->ledRedGpio, 1);
            pLed->blinkCountDown = kFastBlinkCount;
            gTimerOnRequests++;
            break;
#endif

#if defined(GPL_CODE)
        case kLedStateAmberUserWpsInProgress:      /* 200ms on, 100ms off */
//            aei_set_led_brightness(pLed->ledRedGpio, 1); // fall though, only need change red LED when turn to Amber
        case kLedStateRedUserWpsInProgress:      /* 200ms on, 100ms off */
#endif
        case kLedStateUserWpsInProgress:          /* 200ms on, 100ms off */
            pLed->blinkCountDown = kFastBlinkCount;
            gLedRunningCounter = 0;
            gTimerOnRequests++;
            break;             

        case kLedStateUserWpsError:               /* 100ms on, 100ms off */
            pLed->blinkCountDown = kFastBlinkCount;
            gLedRunningCounter = 0;
            gTimerOnRequests++;
            break;        

#if defined(GPL_CODE)
        case kLedStateAmberUserWpsSessionOverLap:  /* 100ms on, 100ms off, 5 times, off for 500ms */
//            aei_set_led_brightness(pLed->ledRedGpio, 1); // fall though, only need change red LED when turn to Amber
#endif
        case kLedStateUserWpsSessionOverLap:      /* 100ms on, 100ms off, 5 times, off for 500ms */
            pLed->blinkCountDown = kFastBlinkCount;
            gTimerOnRequests++;
            break;        
#if defined(GPL_CODE) || defined(GPL_CODE)
        case kLedStateUserWANGreenRedVerySlowBlinkContinues:      /*rotate between green and red in a 1 second on/off interval*/
            pLed->blinkCountDown = kVerySlowBlinkCount;
            gTimerOnRequests++;
            break;
        case kLedStatePowerOneSecondBlinkContinues:
            pLed->blinkCountDown = kOneSecondBlinkCount;
            gTimerOnRequests++;
            break;
#endif
        default:
            printk("Invalid led state\n");
        }
    }

    // If gTimerOn is false, that means ledTimerExpire has already finished
    // running and has not restarted the timer.  Then we can start it here.
    // Otherwise, we only leave the gTimerOnRequests > 0 which will be
    // noticed at the end of the ledTimerExpire function.
    if (!gTimerOn && gTimerOnRequests > 0)
    {
        ledTimerStart();
        gTimerOn = TRUE;
        gTimerOnRequests = 0;
    }
    spin_unlock_irqrestore(&brcm_ledlock, flags);
}

#if defined(GPL_CODE)
void AEI_boardLedBrightnessCtrl(BOARD_LED_NAME ledName, unsigned int ledState)
{
    unsigned long flags;

    if (ledState < 0 || ledState > 8)
    {
        printk("Invalid led state:%u\n", ledState);
        return;
    }
    if( (int) ledName < kLedEnd ) {
        BCM_ASSERT_NOT_HAS_SPINLOCK_V(&brcm_ledlock);
        spin_lock_irqsave(&brcm_ledlock, flags);

//        if (ledName == kLedInternet)
//            aei_set_internet_led_brightness(ledState);
//        else if (ledName == kLedSes)
//            aei_set_wifi_led_brightness(ledState);

        spin_unlock_irqrestore(&brcm_ledlock, flags);
    }
}
#endif

#if defined(CONFIG_NEW_LEDS)
#include <linux/leds.h>
#define LED_AUTONAME_MAX_SIZE    25
#define MAX_LEDS 64


struct sysfsled {
    struct led_classdev cdev;
    int bcm_led_pin; /* This also means pins connected on a shift
    register controlled by the LED controller (see BP_GPIO_SERIAL).*/
    char name[LED_AUTONAME_MAX_SIZE];
};

struct sysfsled_ctx {
    int    num_leds;
    struct sysfsled leds[MAX_LEDS];
};

static struct sysfsled_ctx bcmctx = {0, {}};

static void brightness_set(struct led_classdev *cdev, enum led_brightness value)
{
    struct sysfsled *led = container_of(cdev,
                    struct sysfsled, cdev);

    /* On Broadcom chips, LEDs connected to any kind of pin can be
     controlled via setLed. setLed only needs the pin ("GPIO") from
     the LED_CTRL argument. Fake a LED_CTRL to pass the pin to setLed.
     The color obviously doesn't matter here. */
    LED_CTRL ledctrl = {.ledGreenGpio=led->bcm_led_pin};
    setLed(&ledctrl, (value == LED_FULL)?kLedOn:kLedOff, kLedGreen);
}

static void __exit bcmsysfsleds_unregister(void *opaque)
{
    struct sysfsled_ctx *ctx = opaque;
    int i;

    for (i = 0; i < MAX_LEDS; i++) {
        if (ctx->leds[i].cdev.brightness_set) {
            led_classdev_unregister(&ctx->leds[i].cdev);
            ctx->leds[i].cdev.brightness_set = NULL;
        }
    }
}

static int __init bcmsysfsleds_register(int bcm_led_pin, char *led_name)
{
    int i;

    if (bcmctx.num_leds >= MAX_LEDS) {
        printk(KERN_ERR "Too many BCM LEDs registered.\n");
        return -ENOMEM;
    }

    if(led_name != NULL) {
        i = bcmctx.num_leds;
        snprintf(bcmctx.leds[i].name, LED_AUTONAME_MAX_SIZE-1,
            "%s_%d%c", led_name, bcm_led_pin&BP_GPIO_NUM_MASK, (bcm_led_pin&BP_GPIO_SERIAL)?'s':'\0');
        bcmctx.leds[i].cdev.name = bcmctx.leds[i].name;
        bcmctx.leds[i].cdev.brightness  = LED_OFF;
        bcmctx.leds[i].cdev.brightness_set = brightness_set;
        bcmctx.leds[i].bcm_led_pin = bcm_led_pin;

        if (led_classdev_register(NULL, &bcmctx.leds[i].cdev)) {
            printk(KERN_ERR "BCM LEDs registration failed. %d\n", bcm_led_pin&BP_GPIO_NUM_MASK);
            bcmctx.leds[i].cdev.brightness_set = NULL;
            return -1;
        }
        bcmctx.num_leds++;
    }

    i = bcmctx.num_leds;
    snprintf(bcmctx.leds[i].name, LED_AUTONAME_MAX_SIZE-1,
        "%d%c", bcm_led_pin&BP_GPIO_NUM_MASK, (bcm_led_pin&BP_GPIO_SERIAL)?'s':'\0');
    bcmctx.leds[i].cdev.name = bcmctx.leds[i].name;
    bcmctx.leds[i].cdev.brightness  = LED_OFF;
    bcmctx.leds[i].cdev.brightness_set = brightness_set;
    bcmctx.leds[i].bcm_led_pin = bcm_led_pin;
    if (led_classdev_register(NULL, &bcmctx.leds[i].cdev)) {
        printk(KERN_ERR "BCM LEDs registration failed. %d\n", bcm_led_pin&BP_GPIO_NUM_MASK);
        bcmctx.leds[i].cdev.brightness_set = NULL;
        return -1;
    }

    bcmctx.num_leds++;

    return 0;
}
#endif

int __init bcmsysfsleds_init(void)
{
#if defined(CONFIG_NEW_LEDS)
        unsigned short bcmledgpioid;
	int index=0, token=0, rc;
        char *ledName=NULL;
        for(;;) {
                ledName=NULL;
#if defined(GPL_CODE_63138_CHIP)
#if defined(GPL_CODE) || defined(GPL_CODE)
                // In C3000 and T3200 project, there is same GPIP between bp_usGpioLedWifi, bp_usGpioLedWifiFail and bp_usGpioLedSesWireless, bp_usGpioLedSesWirelessFail, 
                // so sysfs create led filename will fail and kernel prink panic.
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
                if (BpisWPSLEDGPIO(index) || BpisInternetLEDGPIO(index))
#else
                if (BpisWPSLEDGPIO(index))
#endif
                {
                    index++;
                    token=0;
                    continue;
                }
#endif
#endif
                rc = BpGetLedName(index, &token,  &bcmledgpioid, &ledName);
                if (rc == BP_MAX_ITEM_EXCEEDED) {
                    break;
                }
                if(rc == BP_SUCCESS) {
                        bcmsysfsleds_register(bcmledgpioid, ledName);
                }
                else {
                        index++;
                        token=0;
                }
        }
#endif
        return 0;
}

void __exit bcmsysfsleds_exit(void)
{
#if defined(CONFIG_NEW_LEDS)
    int i;

    for (i = 0; i < MAX_LEDS; i++) {
        if (bcmctx.leds[i].cdev.brightness_set) {
            led_classdev_unregister(&bcmctx.leds[i].cdev);
            bcmctx.leds[i].cdev.brightness_set = NULL;
        }
    }
#endif
}

module_init(bcmsysfsleds_init);

#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
/*
 * Turn on internet red LED with low-light indicate userspace app crash;
 * Turn on internet red LED with high-light indicate kernel crash.
 */
void aei_set_internet_red_led(unsigned int is_highlight)
{
    unsigned short gpio = 0;

    if (gLedCtrl) { /*if brcm_led has init*/
        boardLedCtrl(kLedInternet, kLedStateFail);
    } else {
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM960333) || defined(CONFIG_BCM96848)
        if (BpGetInternetRedLedGpio( &gpio ) == BP_SUCCESS) {
            aei_flash_led(gpio, 0);
            bcm_led_driver_set(gpio, 1);
        }

        if (BpGetInternetGreenLedGpio( &gpio ) == BP_SUCCESS) {
            bcm_led_driver_set(gpio, 0);
        }
#endif
    }
    if (BpGetInternetRedLedGpio( &gpio ) == BP_SUCCESS) {
        if (is_highlight) {
            aei_set_led_brightness(gpio, 0x8);
        } else {
            aei_set_led_brightness(gpio, 0x1);
        }
    }
}
EXPORT_SYMBOL(aei_set_internet_red_led);
#endif

#if defined(GPL_CODE)
void aei_set_internet_led_brightness(unsigned int brightness)
{
    unsigned short gpio_green = 0u;
    unsigned short gpio_red = 0u;

//    if (BpGetInternetRedLedGpio( &gpio_red ) == BP_SUCCESS &&
//        BpGetInternetGreenLedGpio( &gpio_green ) == BP_SUCCESS)
//    {
//        aei_set_led_brightness(gpio_green, brightness);
//        aei_set_led_brightness(gpio_red, brightness);
//    }
}

void aei_set_wifi_led_brightness(unsigned int brightness)
{
    unsigned short gpio_green = 0u;
    unsigned short gpio_red = 0u;

//    if (BpGetWifiLedGpio( &gpio_green ) == BP_SUCCESS &&
//        BpGetWifiFailLedGpio( &gpio_red ) == BP_SUCCESS)
    {
//        aei_set_led_brightness(gpio_green, brightness);
//        aei_set_led_brightness(gpio_red, brightness);
    }
}
#endif
