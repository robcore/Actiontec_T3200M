/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2011:DUAL/GPL:standard
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * 
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
 * writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
:>
 *
 ************************************************************************/

#ifndef __CMS_LED_H__
#define __CMS_LED_H__


/*!\file cms_led.h
 * \brief Header file for the CMS LED API.
 *  This is in the cms_util library.
 *
 */

#include "cms.h"


void cmsLed_setWanConnected(void);

void cmsLed_setWanDisconnected(void);

#if defined(GPL_CODE)
void cmsLed_setWanFailed(void);
#endif

#if defined(GPL_CODE)
void cmsLed_setWifiLedOn(void);

void cmsLed_setWifiLedOff(void);

void cmsLed_setWifiLedBlink(void);
#endif

#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR) || defined(GPL_CODE)
#define AEI_LED_OFF    0
#define AEI_LED_GREEN  1
#define AEI_LED_RED    2
#define AEI_LED_YELLOW 3
#define AEI_LED_GREEN_BLINK  4
#define AEI_LED_YELLOW_BLINK  5
#define AEI_LED_WPS_IN_PROGRESS_GREEN 6
#define AEI_LED_WPS_IN_PROGRESS_RED 7
#define AEI_LED_WPS_IN_PROGRESS_YELLOW 8
#define AEI_LED_WPS_ERROR  9
#define AEI_LED_WPS_SESSION_OVER_LAP_GREEN  10
#define AEI_LED_WPS_SESSION_OVER_LAP_YELLOW  11
#define AEI_LED_WIFI_OFF    12
#define AEI_LED_WIFI_GREEN  13
#define AEI_LED_WIFI_RED    14
#define AEI_LED_WIFI_YELLOW 15

void update_wifi_led_state(unsigned int state);
void update_led_brightness(char *led_name, unsigned int brightness);
#endif

#if defined(GPL_CODE)
#define AUTODETECTMACK "/var/autodetectmack"
#endif

#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
void update_internet_led_state(unsigned int state);
#endif
#endif /* __CMS_LED_H__ */
