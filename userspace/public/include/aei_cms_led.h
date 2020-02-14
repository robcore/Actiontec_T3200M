/***********************************************************************
 *
 *  Copyright (c) 2011  Actiontec Electronics Inc.
 *  All Rights Reserved
 *
 *  This file is to store all functions that developed by Actiontec Electronics
 *  in addition to routines provided by Broadcom. All additional routines that
 *  are missing from cms_led.h file will locate in this file.
 *
 ************************************************************************/

#ifndef __AEI_CMS_LED_H__
#define __AEI_CMS_LED_H__

#if defined(GPL_CODE)
#include "aei_cms_msg.h"
#include "board.h"

int AEI_boardIoctl(int boardFd, int board_ioctl, BOARD_IOCTL_ACTION action,
                   char *string, int strLen, int offset);

#if defined(GPL_CODE_SMARTLED)
UBOOL8 convertOptToLedCase(InetLedControlBody *pLedCtlBody, InetLedCase *pLedCtlCase);
void AEI_setInetLedTrafficBlink(int state);
#endif

void AEI_cmsLed_setPowerGreen(void);

#if defined(SUPPORT_DSL_BONDING) && defined(GPL_CODE)
void AEI_cmsLed_setWanLineStatus(int state);
#endif
void AEI_cmsLed_setWanAmber(void);
void AEI_cmsLed_setEthWanConnected(void);
void AEI_cmsLed_setEthWanDisconnected(void);
#if defined(GPL_CODE)
void AEI_cmsLed_setPowerAmber(void);
void AEI_cmsLed_setWanDetect(void);
void AEI_cmsLed_setWanOff(void);
void cmsLed_setWanFailed_Autodetect(void);
#endif
#endif /* GPL_CODE */
#endif /* __AEI_CMS_LED_H__ */
