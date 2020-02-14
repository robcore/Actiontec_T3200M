/***********************************************************************
 *
 *  Copyright (c) 2011  Actiontec Electronics Inc.
 *  All Rights Reserved
 *
 *  This file is to store all functions that developed by Actiontec Electronics
 *  in addition to routines provided by Broadcom. All additional routines that
 *  are missing from led.c file will locate in this file.
 *
 ************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "aei_cms_msg.h"

#if defined(GPL_CODE)
int AEI_boardIoctl(int boardFd, int board_ioctl, BOARD_IOCTL_ACTION action,
                   char *string, int strLen, int offset)
{
    BOARD_IOCTL_PARMS IoctlParms;

    IoctlParms.string = string;
    IoctlParms.strLen = strLen;
    IoctlParms.offset = offset;
    IoctlParms.action = action;

    ioctl(boardFd, board_ioctl, &IoctlParms);

    return (IoctlParms.result);
}

void AEI_sysGPIOCtrl(int gpio, GPIO_STATE_t state)
{
    int boardFd;

    if ((boardFd = open("/dev/brcmboard", O_RDWR)) != -1)
    {
        AEI_boardIoctl(boardFd, BOARD_IOCTL_SET_GPIO, 0, "", (int)gpio, (int)state);
        close(boardFd);
    }
    else
        printf("Unable to open device /dev/brcmboard.\n");
}

void AEI_cmsLed_setPowerGreen(void)
{
#if defined(GPL_CODE) && !defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedPower, kLedStateOn, NULL);
#endif
}

#if defined(GPL_CODE)
void AEI_cmsLed_setPowerAmber(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedPower, kLedStateAmber, NULL);
}

void cmsLed_setWanFailed_Autodetect(void)
{
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedInternet, kLedStateFail, NULL);
#else
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWanData, kLedStateFail, NULL);
#endif
}

void AEI_cmsLed_setWanOff(void)
{
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
    devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedInternet, kLedStateOff, NULL);
#else
    devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWanData, kLedStateOff, NULL);
#endif
}

void AEI_cmsLed_setWanDetect(void)
{
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedInternet, kLedStateUserWANGreenRedVerySlowBlinkContinues, NULL);
#else
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWanData, kLedStateUserWANGreenRedVerySlowBlinkContinues, NULL);
#endif
}
#endif

void AEI_cmsLed_setWanAmber(void)
{
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR) && defined(GPL_CODE)
   update_internet_led_state(kLedStateAmber);
#else
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWanData, kLedStateAmber, NULL);
#endif
}

void AEI_cmsLed_setEthWanConnected(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedSecAdsl, kLedStateOn, NULL);
}
void AEI_cmsLed_setEthWanDisconnected(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedSecAdsl, kLedStateOff, NULL);
}

#if defined(SUPPORT_DSL_BONDING) && defined(GPL_CODE)

void AEI_cmsLed_setWanLineStatus(int state)
{
/*
 need to be done
    int boardFd;

    if ((boardFd = open("/dev/bcmadsl0", O_RDWR)) == -1)
        printf("Unable to open device /dev/bcmadsl0.\n");
    else
        ioctl(boardFd,ADSLIOCTL_SET_DSL_WAN_STATUS,state);
    close(boardFd);
*/
}
#endif

#if defined(GPL_CODE_SMARTLED)
UBOOL8 convertOptToLedCase(InetLedControlBody *pLedCtlBody, InetLedCase *pLedCtlCase)
{
    if (pLedCtlBody->color == INET_LED_OFF)
    {
        *pLedCtlCase = CMS_MSG_SET_INET_LED_OFF;
    }
    else if (pLedCtlBody->color == INET_LED_RED)
    {
        if (pLedCtlBody->action == INET_LED_NONE)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_RED;
        else if (pLedCtlBody->action == INET_LED_FLASH)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_RED_FLASH;
        else if (pLedCtlBody->action == INET_LED_BLINK)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_RED_BLINK;
    }
    else if (pLedCtlBody->color == INET_LED_GREEN)
    {
        if (pLedCtlBody->action == INET_LED_NONE)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_GREEN;
        else if (pLedCtlBody->action == INET_LED_FLASH)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_GREEN_FLASH;
        else if (pLedCtlBody->action == INET_LED_BLINK)
            *pLedCtlCase = CMS_MSG_SET_INET_LED_GREEN_BLINK;
    }
    else if (pLedCtlBody->color == INET_LED_AMBER)
    {
        if (pLedCtlBody->action == INET_LED_NONE)
           *pLedCtlCase = CMS_MSG_SET_INET_LED_AMBER;
        else if (pLedCtlBody->action == INET_LED_FLASH)
           *pLedCtlCase = CMS_MSG_SET_INET_LED_AMBER_FLASH;
        else if (pLedCtlBody->action == INET_LED_BLINK)
           *pLedCtlCase = CMS_MSG_SET_INET_LED_AMBER_BLINK;
        else if (pLedCtlBody->action == INET_LED_ALTER)
           *pLedCtlCase = CMS_MSG_SET_INET_LED_RED_GREEN_ALTERNATE;
    }

    if (*pLedCtlCase == CMS_MSG_SET_INET_LED_NONE)
        return FALSE;
    else
        return TRUE;
}
#endif

#endif /* GPL_CODE */
