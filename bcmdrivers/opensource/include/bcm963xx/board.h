/*
<:copyright-BRCM:2002:GPL/GPL:standard

   Copyright (c) 2002 Broadcom Corporation
   All Rights Reserved

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
/***********************************************************************/
/*                                                                     */
/*   MODULE:  board.h                                                  */
/*   PURPOSE: Board specific information.  This module should include  */
/*            all base device addresses and board specific macros.     */
/*                                                                     */
/***********************************************************************/
#ifndef _BOARD_H
#define _BOARD_H

#include "bcm_hwdefs.h"
#include <boardparms.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BOARD_H_API_VER increases when other modules (such as PHY) depend on */
/* a new function in the board driver or in boardparms.h                */

#define BOARD_H_API_VER 10

/*****************************************************************************/
/*          board ioctl calls for flash, led and some other utilities        */
/*****************************************************************************/
/* Defines. for board driver */
#define BOARD_IOCTL_MAGIC       'B'
#define BOARD_DRV_MAJOR          206

/** Device filename of ioctl device. */
#define BOARD_DEVICE_NAME  "/dev/brcmboard"

#define BOARD_IOCTL_FLASH_WRITE                 _IOWR(BOARD_IOCTL_MAGIC, 0, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_FLASH_READ                  _IOWR(BOARD_IOCTL_MAGIC, 1, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_DUMP_ADDR                   _IOWR(BOARD_IOCTL_MAGIC, 2, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_MEMORY                  _IOWR(BOARD_IOCTL_MAGIC, 3, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_MIPS_SOFT_RESET             _IOWR(BOARD_IOCTL_MAGIC, 4, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_LED_CTRL                    _IOWR(BOARD_IOCTL_MAGIC, 5, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_PSI_SIZE                _IOWR(BOARD_IOCTL_MAGIC, 6, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_SDRAM_SIZE              _IOWR(BOARD_IOCTL_MAGIC, 7, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_ID                      _IOWR(BOARD_IOCTL_MAGIC, 8, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_CHIP_ID                 _IOWR(BOARD_IOCTL_MAGIC, 9, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_CHIP_REV                _IOWR(BOARD_IOCTL_MAGIC, 10, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_CFE_VER                 _IOWR(BOARD_IOCTL_MAGIC, 11, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_BASE_MAC_ADDRESS        _IOWR(BOARD_IOCTL_MAGIC, 12, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_MAC_ADDRESS             _IOWR(BOARD_IOCTL_MAGIC, 13, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_RELEASE_MAC_ADDRESS         _IOWR(BOARD_IOCTL_MAGIC, 14, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NUM_ENET_MACS           _IOWR(BOARD_IOCTL_MAGIC, 15, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NUM_ENET_PORTS          _IOWR(BOARD_IOCTL_MAGIC, 16, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_MONITOR_FD              _IOWR(BOARD_IOCTL_MAGIC, 17, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_WAKEUP_MONITOR_TASK         _IOWR(BOARD_IOCTL_MAGIC, 18, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_TRIGGER_EVENT           _IOWR(BOARD_IOCTL_MAGIC, 19, BOARD_IOCTL_PARMS)        
#define BOARD_IOCTL_GET_TRIGGER_EVENT           _IOWR(BOARD_IOCTL_MAGIC, 20, BOARD_IOCTL_PARMS)        
#define BOARD_IOCTL_UNSET_TRIGGER_EVENT         _IOWR(BOARD_IOCTL_MAGIC, 21, BOARD_IOCTL_PARMS) 
#define BOARD_IOCTL_GET_WLAN_ANT_INUSE          _IOWR(BOARD_IOCTL_MAGIC, 22, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_SES_LED                 _IOWR(BOARD_IOCTL_MAGIC, 23, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_CS_PAR                  _IOWR(BOARD_IOCTL_MAGIC, 25, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_GPIO                    _IOWR(BOARD_IOCTL_MAGIC, 26, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_FLASH_LIST                  _IOWR(BOARD_IOCTL_MAGIC, 27, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_BACKUP_PSI_SIZE         _IOWR(BOARD_IOCTL_MAGIC, 28, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_SYSLOG_SIZE             _IOWR(BOARD_IOCTL_MAGIC, 29, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_SHUTDOWN_MODE           _IOWR(BOARD_IOCTL_MAGIC, 30, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_STANDBY_TIMER           _IOWR(BOARD_IOCTL_MAGIC, 31, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_BOOT_IMAGE_OPERATION        _IOWR(BOARD_IOCTL_MAGIC, 32, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_TIMEMS                  _IOWR(BOARD_IOCTL_MAGIC, 33, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_GPON_OPTICS_TYPE        _IOWR(BOARD_IOCTL_MAGIC, 34, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_DEFAULT_OPTICAL_PARAMS  _IOWR(BOARD_IOCTL_MAGIC, 35, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_ALLOC_MAC_ADDRESSES         _IOWR(BOARD_IOCTL_MAGIC, 36, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_RELEASE_MAC_ADDRESSES       _IOWR(BOARD_IOCTL_MAGIC, 37, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_INIT              _IOWR(BOARD_IOCTL_MAGIC, 38, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_READ              _IOWR(BOARD_IOCTL_MAGIC, 39, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_WRITE             _IOWR(BOARD_IOCTL_MAGIC, 40, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NUM_FE_PORTS            _IOWR(BOARD_IOCTL_MAGIC, 41, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NUM_GE_PORTS            _IOWR(BOARD_IOCTL_MAGIC, 42, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NUM_VOIP_PORTS          _IOWR(BOARD_IOCTL_MAGIC, 43, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_SWITCH_PORT_MAP         _IOWR(BOARD_IOCTL_MAGIC, 44, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_PORT_MAC_TYPE           _IOWR(BOARD_IOCTL_MAGIC, 45, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_EPON_GPIOS              _IOWR(BOARD_IOCTL_MAGIC, 46, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_SET_BITS          _IOWR(BOARD_IOCTL_MAGIC, 47, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_CLEAR_BITS        _IOWR(BOARD_IOCTL_MAGIC, 48, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SPI_SLAVE_WRITE_BUF         _IOWR(BOARD_IOCTL_MAGIC, 49, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_GPIOVERLAYS             _IOWR(BOARD_IOCTL_MAGIC, 50, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_BATTERY_ENABLE          _IOWR(BOARD_IOCTL_MAGIC, 51, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_FTTDP_DSP_BOOTER            _IOWR(BOARD_IOCTL_MAGIC, 52, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_MEM_ACCESS                  _IOWR(BOARD_IOCTL_MAGIC, 53, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_DYING_GASP_INTERRUPT    _IOWR(BOARD_IOCTL_MAGIC, 54, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_BTRM_BOOT               _IOWR(BOARD_IOCTL_MAGIC, 55, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_BOOT_SECURE             _IOWR(BOARD_IOCTL_MAGIC, 56, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SATA_TEST                   _IOWR(BOARD_IOCTL_MAGIC, 57, BOARD_IOCTL_PARMS)

#if defined(GPL_CODE)
#define BASE_AEI_IOCPRIVATE	60
#define BOARD_IOCTL_GET_SN              _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 1, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_POWER_LED_ON        _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 2, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_POWER_LED_FLASH     _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 3, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_NVRAM_PARAM     _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 4, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_RESET_NVRAM_PARAM   _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 5, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_NVRAM_PARAM     _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 6, BOARD_IOCTL_PARMS)

#if defined(GPL_CODE)
#define AEI_BOARD_IOCTL_LED_BRIGHTNESS_CTRL     _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 7, BOARD_IOCTL_PARMS)
#endif

typedef enum
{
    AEI_NVPARAM_SN = 0,   //ulSerialNumber
    AEI_NVPARAM_FW,       //chFactoryFWVersion
    AEI_NVPARAM_WPS_PIN,  //wpsPin
    AEI_NVPARAM_WPA_KEY,  //wpaKey
    AEI_NVPARAM_ADMIN_PW, //adminPassword
    AEI_NVPARAM_BOOTLINE, //szBootline
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
    AEI_NVPARAM_WLAN_FEATURE, //wlan_feature; the last digit of wlanParams. 
#endif
     AEI_NVPARAM_HW_VERSION,
     AEI_NVPARAM_BASE_MAC_ADDRESS,
#ifdef GPL_CODE_FACTORY_TEST
     AEI_NVPARAM_BURN_IN_TEST,
     AEI_NVPARAM_MANU_MODE,
     AEI_NVPARAM_ENABLE_BACKUP_PSI,
     AEI_NVPARAM_SYSTEM_LOG_SIZE,
     AEI_NVPARAM_AUXILLARY_FILE_SYSTEM_SIZE,
     AEI_NVPARAM_MAIN_THREAD_NUMBER,
     AEI_NVPARAM_NUMBER_MAC_ADDRESSES,
     AEI_NVPARAM_WLANFEATURE,
     AEI_NVPARAM_PSI_SIZE,
     AEI_NVPARAM_VOICE_BOARD_ID,
     AEI_NVPARAM_FACTORY_FW_VERSION,
#endif
#ifdef GPL_CODE_DETECT_BOARD_ID
    AEI_NVPARAM_DETECT_BID,
#endif
#ifdef GPL_CODE_FACTORY_TEST
    AEI_NVPARAM_RDP_TM_SIZE,
    AEI_NVPARAM_RDP_MC_SIZE,
    AEI_NVPARAM_DHD_MEM_0_ZISE,
    AEI_NVPARAM_DHD_MEM_1_ZISE,
    AEI_NVPARAM_DHD_MEM_2_ZISE,
#endif
}AEI_NVRAM_PARAM_INDEX;


#if defined(GPL_CODE_SMARTLED)
#define BOARD_IOCTL_SET_INET            _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 7, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_INET_TRAFFIC_BLINK _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 8, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE_CONFIG_JFFS) || defined(GPL_CODE_UPGRADE_DUALIMG_HISTORY_SPAD) || defined(GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG)
#define BOARD_IOCTL_GET_DUAL_FW_VERSION _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 9, BOARD_IOCTL_PARMS)
#endif
#define BOARD_IOCTL_GET_UG_INFO _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 10, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_SET_UG_INFO _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 11, BOARD_IOCTL_PARMS)
#if defined(GPL_CODE)
#define BOARD_IOCTL_GET_FS_OFFSET _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 13, BOARD_IOCTL_PARMS)
#endif

#if defined(GPL_CODE)
#define BOARD_IOCTL_GET_POWERLED_STATUS  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 14, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_FLASH_TOTAL  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 15, BOARD_IOCTL_PARMS)

#define BOARD_IOCTL_SET_WLANLEDMODE _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE+17, BOARD_IOCTL_PARMS)
#define BOARD_IOCTL_GET_WLANLEDMODE _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE+18, BOARD_IOCTL_PARMS)
#define AEI_BOARD_IOCTL_WIRELESS_REDLED    _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE+19, BOARD_IOCTL_PARMS)

#endif
#if defined(GPL_CODE_FACTORY_TEST)
#define AEI_BOARD_IOCTL_SET_RMA_STATUS  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 20, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE)
#define BOARD_IOCTL_ADJUST_NVRAM      _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 21, BOARD_IOCTL_PARMS)
#endif
#endif
#if defined(GPL_CODE_DEBUG_NVRAM)
#define BOARD_IOCTL_PRINT_NVRAM 	  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 25, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE_VOIP_LED)
#define AEI_BOARD_IOCTL_VOIP_LED _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 22, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE_CHECK_FLASH_ID)
#define AEI_BOARD_IOCTL_GET_NAND_FLASH_ID  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 23, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE_FACTORY_TELNET)
#define AEI_BOARD_IOCTL_SET_RESTOREDEFAULT_FLAG  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 25, BOARD_IOCTL_PARMS)
#define AEI_BOARD_IOCTL_GET_RESTOREDEFAULT_FLAG  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 26, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE)
#define AEI_BOARD_IOCTL_SET_WAN_TYPE  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 24, BOARD_IOCTL_PARMS)
#endif

#ifdef GPL_CODE_NAND_CNT_128K
#define AEI_BOARD_IOCTL_GET_FLASH_CNT  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 27, BOARD_IOCTL_PARMS)
#endif

#if defined(GPL_CODE)
#define AEI_BOARD_IOCTL_SET_NVRAM_DATA  _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 28, BOARD_IOCTL_PARMS)
#endif
#if defined(GPL_CODE)
#define BOARD_IOCTL_GET_HW_VERSION        _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 29,  BOARD_IOCTL_PARMS)

#ifdef GPL_CODE_FACTORY_TEST
#define NVRAM_AEI_BURN_IN_TEST_TAG "aei_burn_in_test"
#define NVRAM_AEI_MANU_MODE_TAG "aei_manu_mode"
#define NVRAM_AEI_WPA_KEY_TAG "aei_wpa_key"
#define NVRAM_AEI_WPS_AP_PIN_TAG "aei_wps_ap_pin"
#define NVRAM_AEI_ADMIN_PASSWORD_TAG "aei_admin_password"
#define NVRAM_AEI_SN_TAG "aei_serial_number"
#define NVRAM_AEI_HW_VERSION_TAG "aei_hw_version"
#define NVRAM_AEI_NUMBER_MAC_ADDRESS_TAG "aei_number_mac_address"
#define NVRAM_AEI_BASE_MAC_ADDRESS_TAG "aei_base_mac_address"
#define NVRAM_AEI_PSI_SIZE_TAG "aei_psi_size"
#define NVRAM_AEI_BACKUP_PSI_TAG "aei_backup_psi"
#define NVRAM_AEI_SYSTEM_LOG_SIZE_TAG "aei_system_log_size"
#define NVRAM_AEI_FACTORY_FW_VERSION_TAG "aei_factory_FW_version"
#define NVRAM_AEI_WLAN_FEATURE_TAG "aei_wlan_feature"
#define NVRAM_AEI_RDP_TM_SIZE_TAG "aei_rdp_tm_size"
#define NVRAM_AEI_RDP_MC_SIZE_TAG "aei_rdp_mc_size"
#define NVRAM_AEI_DHD_MEM_0_SIZE_TAG "aei_dhd_mem_0_size"
#define NVRAM_AEI_DHD_MEM_1_SIZE_TAG "aei_dhd_mem_1_size"
#define NVRAM_AEI_DHD_MEM_2_SIZE_TAG "aei_dhd_mem_2_size"
#endif
#ifdef GPL_CODE_DETECT_BOARD_ID
#define NVRAM_AEI_AUTO_DETECT_BID_TAG "aei_detect_bid"
#endif
#endif

#if defined(GPL_CODE_63168_CHIP)
#define AEI_BOARD_IOCTL_GET_FLASH_DEVICE _IOWR(BOARD_IOCTL_MAGIC, BASE_AEI_IOCPRIVATE + 30,  BOARD_IOCTL_PARMS)
#endif

#if defined(GPL_CODE)
typedef enum
{
    AEI_BOARD_UNKNOWN,
    AEI_BOARD_C1000A,
    AEI_BOARD_C1000B,
    AEI_BOARD_C2000A,
    AEI_BOARD_C3000A,
    AEI_BOARD_V1000H,
    AEI_BOARD_V2000H,
    AEI_BOARD_V1000W,
    AEI_BOARD_R1000H,
    AEI_BOARD_FV2200,
    AEI_BOARD_FV2210,
    AEI_BOARD_FV2250,
    AEI_BOARD_V2200H,
    AEI_BOARD_T2200H,
    AEI_BOARD_T3200,
    AEI_BOARD_T1200H,
    AEI_BOARD_F3500,
    AEI_BOARD_END
} AEI_BOARD_ID;

#define BP_ACTIVE_HIGH                          0x0000
#define BP_ACTIVE_LOW                           0x8000

#if 0
#if defined(GPL_CODE_63168_CHIP) // For 63168
  #define AEI_POWER_ON_LED    (BP_ACTIVE_LOW | 20)
  #define AEI_POST_FAIL_LED   (BP_ACTIVE_LOW | 35)
  #define AEI_INET_ACT_LED    (BP_ACTIVE_LOW | 13)
  #define AEI_INET_FAIL_LED   (BP_ACTIVE_LOW | 12)
#else  // For 6368
  #define AEI_POWER_ON_LED    (BP_ACTIVE_HIGH | 22)
  #define AEI_POST_FAIL_LED   (BP_ACTIVE_HIGH | 24)
  #define AEI_INET_ACT_LED    (BP_ACTIVE_HIGH | 5)
  #define AEI_INET_FAIL_LED   (BP_ACTIVE_HIGH | 31)
#endif
#endif
#endif // GPL_CODE

typedef enum
{
    AEI_WAN_NONE,
    AEI_WAN_XDSL,
    AEI_WAN_ETHERNET,
    AEI_WAN_SFP,
    AEI_WAN_END
} AEI_WAN_TYPE;

// for the action in BOARD_IOCTL_PARMS for flash operation
typedef enum 
{
    PERSISTENT,
    NVRAM,
    BCM_IMAGE_CFE,
    BCM_IMAGE_FS,
    BCM_IMAGE_KERNEL,
    BCM_IMAGE_WHOLE,
    SCRATCH_PAD,
    FLASH_SIZE,
    SET_CS_PARAM,
    BACKUP_PSI,
    SYSLOG,
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
    CUSTOMER_PSI,
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
    OLD_IMAGE_PSI,
#endif
#if defined(GPL_CODE)
    SYSLOGONREBOOT,
#endif
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
    AEI_IMAGE1_PSI,
    AEI_IMAGE2_PSI,
#endif
#ifdef GPL_CODE_NAND_CNT_128K
    AEI_GET_FLASH_CNT_BUF_SIZE,
    AEI_GET_FLASH_CNT
#endif
} BOARD_IOCTL_ACTION;
    
#ifdef GPL_CODE_NAND_CNT_128K
typedef enum 
{
	GET_FLASH_CNT_NVRAM,
	GET_FLASH_CNT_ROOTFS,
	GET_FLASH_CNT_ROOTFS_UPDATE,
	GET_FLASH_CNT_DATA,
	GET_FLASH_CNT_TAG,
	GET_FLASH_CNT_TAG_UPDATE
}FLASH_CNT_AREA;

typedef struct blk_erase_info
{
	unsigned int blk_no;
	unsigned int blk_addr;
	unsigned int erase_time;
}blk_erase_info_t;

typedef struct part_erase_info
{
	char part_name[32];
	unsigned int blk_size;
	unsigned int tlt_size;
	unsigned int blk_cnt;
	unsigned int tlt_erase_time;
	blk_erase_info_t *blk_info;
	
}part_erase_info_t;

#endif

typedef struct boardIoctParms
{
    char *string;
    char *buf;
    int strLen;
    int offset;
    BOARD_IOCTL_ACTION  action;        /* flash read/write: nvram, persistent, bcm image */
    int result;
} BOARD_IOCTL_PARMS;

#define BOARD_MEMACCESS_IOCTL_OP_READ  0
#define BOARD_MEMACCESS_IOCTL_OP_WRITE  1
#define BOARD_MEMACCESS_IOCTL_OP_FILL  2
#define BOARD_MEMACCESS_IOCTL_SPACE_REG  0
#define BOARD_MEMACCESS_IOCTL_SPACE_KERN  1
#define BOARD_MEMACCESS_IOCTL_SPACE_MEM  2

typedef struct boardMemaccessIoctlParms
{
    int op;
    int space;
    void *address;
    int size; // 1, 2, or 4
    int count; // number of items
    char *buf;
} BOARD_MEMACCESS_IOCTL_PARMS;


// LED defines 
typedef enum
{   
    kLedAdsl,
    kLedSecAdsl,
    kLedWanData,
    kLedSes,
    kLedVoip,
    kLedVoip1,
    kLedVoip2,
    kLedPots,
    kLedDect,
    kLedGpon,
    kLedMoCA,
    kLedEth0Duplex,
    kLedEth0Spd100,
    kLedEth0Spd1000,
    kLedEth1Duplex,
    kLedEth1Spd100,
    kLedEth1Spd1000,
#if defined(GPL_CODE)
    kLedEth2Duplex,
    kLedEth2Spd100,
    kLedEth2Spd1000,
    kLedEth3Duplex,
    kLedEth3Spd100,
    kLedEth3Spd1000,
#endif
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    kLedOpticalLink,
    kLedUSB,
    kLedSim,
    kLedSimITMS,
    kLedEpon,
#endif  
#if defined(GPL_CODE)
    kLedUsb,
#if defined(GPL_CODE)
    AEI_kLedWlan,
    AEI_kLedWlanAct,
    AEI_kLedWlanGreen,
    AEI_kLedWlanRed,
#endif
#if defined(GPL_CODE_63168_CHIP)
    kLedEnetWan,
#endif
#if defined(GPL_CODE_63138_CHIP)
    kLedWanEthLink,
    kLedWanEthSpeed100,
    kLedWanEthSpeed1000,
#endif
    kLedWanSfpLink0,
    kLedWanSfpLink1,
    kLedWifi,
    kLedPower,
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
    kLedInternet,
#endif
#endif
    kLedEnd                             // NOTE: Insert the new led name before this one.
} BOARD_LED_NAME;

typedef enum
{
    kLedStateOff,                        /* turn led off */
    kLedStateOn,                         /* turn led on */
    kLedStateFail,                       /* turn led on red */
#if defined(GPL_CODE)
    kLedStateAmber,                      /* turn green led on , turn red led on*/
    kLedStateVerySlowBlinkContinues,         /* slow blink continues at 1HZ interval */
#endif
    kLedStateSlowBlinkContinues,         /* slow blink continues at 2HZ interval */
    kLedStateFastBlinkContinues,         /* fast blink continues at 4HZ interval */
#if defined(GPL_CODE) || defined(GPL_CODE)
    kLedStateAmberSlowBlinkContinues,
    kLedStateAmberFastBlinkContinues,
#endif
#if defined(GPL_CODE)
    kLedStateAmberAndGreenSlowBlinkContinues,
#endif
#if defined(GPL_CODE) || defined(GPL_CODE)
    kLedStateUserWANGreenRedVerySlowBlinkContinues, /*rotate between green and red in a 1 second on/off interval*/
    kLedStatePowerOneSecondBlinkContinues,          /*rotate between green and off in a 1 second on/off interva*/
#endif
    kLedStateUserWpsInProgress,          /* 200ms on, 100ms off */
#if defined(GPL_CODE)
    kLedStateAmberUserWpsInProgress,          /* 200ms on, 100ms off */
    kLedStateRedUserWpsInProgress,          /* 200ms on, 100ms off */
#endif
    kLedStateUserWpsError,               /* 100ms on, 100ms off */
    kLedStateUserWpsSessionOverLap,       /* 100ms on, 100ms off, 5 times, off for 500ms */
#if defined(GPL_CODE)
    kLedStateAmberUserWpsSessionOverLap,       /* 100ms on, 100ms off, 5 times, off for 500ms */
#endif
} BOARD_LED_STATE;

#if defined(GPL_CODE)
void boardLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState);
void AEI_boardLedBrightnessCtrl(BOARD_LED_NAME ledName, unsigned int ledState);
#endif

typedef void (*HANDLE_LED_FUNC)(BOARD_LED_STATE ledState);

typedef enum
{
    kGpioInactive,
    kGpioActive
} GPIO_STATE_t;

// FLASH_ADDR_INFO is now defined in flash_common.h
#include "flash_common.h"

typedef struct cs_config_pars_tag
{
  int   mode;
  int   base;
  int   size;
  int   wait_state;
  int   setup_time;
  int   hold_time;
} cs_config_pars_t;

typedef enum
{
    MAC_ADDRESS_OP_GET=0,
    MAC_ADDRESS_OP_RELEASE,
    MAC_ADDRESS_OP_MAX
} MAC_ADDRESS_OPERATION;

typedef enum
{
	USB_HOST_FUNC,
	USB_DEVICE_FUNC,
	USB_ALL_FUNC
} USB_FUNCTION;


typedef void (* kerSysMacAddressNotifyHook_t)(unsigned char *pucaMacAddr, MAC_ADDRESS_OPERATION op);

#define UBUS_BASE_FREQUENCY_IN_MHZ  160

#define IF_NAME_ETH    "eth"
#define IF_NAME_USB    "usb"
#define IF_NAME_WLAN   "wl"
#define IF_NAME_MOCA   "moca"
#define IF_NAME_ATM    "atm"
#define IF_NAME_PTM    "ptm"
#define IF_NAME_GPON   "gpon"
#define IF_NAME_EPON   "epon"
#define IF_NAME_VEIP    "veip"

#define MAC_ADDRESS_ANY         (unsigned long) -1

/* A unique mac id is required for a WAN interface to request for a MAC address.
 * The 32bit mac id is formated as follows:
 *     bit 28-31: MAC Address IF type (MAC_ADDRESS_*)
 *     bit 20-27: the unit number. 0 for atm0, 1 for atm1, ...
 *     bit 12-19: the connection id which starts from 1.
 *     bit 0-11:  not used. should be zero.
 */
#define MAC_ADDRESS_PLC         0x10000000
#define MAC_ADDRESS_EPONONU     0x20000000
#define MAC_ADDRESS_EPON        0x40000000
#define MAC_ADDRESS_GPON        0x80000000
#define MAC_ADDRESS_1905        0x90000000
#define MAC_ADDRESS_ETH         0xA0000000
#define MAC_ADDRESS_USB         0xB0000000
#define MAC_ADDRESS_WLAN        0xC0000000
#define MAC_ADDRESS_MOCA        0xD0000000
#define MAC_ADDRESS_ATM         0xE0000000
#define MAC_ADDRESS_PTM         0xF0000000

#define UNUSED_PARAM(a)        (a = a)

#if defined(GPL_CODE)
#define AEI_MAC_ADDRESS_TYPE_MASK 0xF0000000
#define AEI_MAC_ADDRESS_NUM_MASK  0x0FF00000  /*It is used for atmX now*/
#define AEI_MAC_ADDRESS_ID_MASK   0x000FF000  /*It is used for ewan0.X or ptm0.X now*/
#endif

/*****************************************************************************/
/*          Function Prototypes                                              */
/*****************************************************************************/
#if !defined(__ASM_ASM_H)
void dumpaddr( unsigned char *pAddr, int nLen );

extern void kerSysEarlyFlashInit( void );
extern int kerSysGetChipId( void );
char * kerSysGetChipName( char *buf, int len);
extern int kerSysGetDslPhyEnable( void );
extern void kerSysFlashInit( void );
extern void kerSysFlashAddrInfoGet(PFLASH_ADDR_INFO pflash_addr_info);
extern int kerSysCfeVersionGet(char *string, int stringLength);

extern int kerSysNvRamSet(const char *string, int strLen, int offset);
extern void kerSysNvRamGet(char *string, int strLen, int offset);
extern void kerSysNvRamLoad(void * mtd_ptr);
extern void kerSysNvRamGetBootline(char *bootline);
extern void kerSysNvRamGetBootlineLocked(char *bootline);
extern void kerSysNvRamGetBoardId(char *boardId);
extern void kerSysNvRamGetBoardIdLocked(char *boardId);
extern void kerSysNvRamGetBaseMacAddr(unsigned char *baseMacAddr);
extern unsigned long kerSysNvRamGetVersion(void);

extern void kerSysDisableDyingGaspInterrupt( void );
extern void kerSysEnableDyingGaspInterrupt( void );

extern int kerSysFsFileSet(const char *filename, char *buf, int len);
extern int kerSysPersistentGet(char *string, int strLen, int offset);
extern int kerSysPersistentSet(char *string, int strLen, int offset);
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
extern int kerSysCustomerPsiGet(char *string, int strLen, int offset);
extern int kerSysCustomerPsiSet(char *string, int strLen, int offset);
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
extern int kerSysOldImageCfgActive(void);
extern int kerSysOldImageCfgSet(char *string, int strLen);
#endif
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
extern int AEI_kerSysImagePsiGet(BOARD_IOCTL_ACTION action,char *string, int strLen, int offset);
extern int AEI_kerSysImagePsiSet(BOARD_IOCTL_ACTION action,char *string, int strLen, int offset);
#endif 
extern int kerSysBackupPsiGet(char *string, int strLen, int offset);
extern int kerSysBackupPsiSet(char *string, int strLen, int offset);
extern int kerSysSyslogGet(char *string, int strLen, int offset);
extern int kerSysSyslogSet(char *string, int strLen, int offset);
extern int kerSysScratchPadList(char *tokBuf, int tokLen);
extern int kerSysScratchPadGet(char *tokName, char *tokBuf, int tokLen);
extern int kerSysScratchPadSet(char *tokName, char *tokBuf, int tokLen);
#if defined(GPL_CODE)
extern int kerClearScratchPad(int blk_size);
#endif
extern int kerSysScratchPadClearAll(void);
extern int kerSysBcmImageSet( int flash_start_addr, char *string, int size,
    int should_yield);
extern int kerSysBcmNandImageSet( char *rootfs_part, char *string, int img_size,
    int should_yield );
extern int kerSysSetBootImageState( int state );
extern int kerSysGetBootImageState( void );
extern int kerSysSetOpticalPowerValues(unsigned short rxReading, unsigned short rxOffset, 
    unsigned short txReading);
extern int kerSysGetOpticalPowerValues(unsigned short *prxReading, unsigned short *prxOffset, 
    unsigned short *ptxReading);
extern int kerSysBlParmsGetInt( char *name, int *pvalue );
extern int kerSysBlParmsGetStr( char *name, char *pvalue, int size );
extern unsigned long kerSysGetMacAddressType( unsigned char *ifName );
extern int kerSysMacAddressNotifyBind(kerSysMacAddressNotifyHook_t hook);
extern int kerSysGetMacAddress( unsigned char *pucaAddr, unsigned long ulId );
extern int kerSysReleaseMacAddress( unsigned char *pucaAddr );
extern void kerSysGetGponSerialNumber( unsigned char *pGponSerialNumber);
extern void kerSysGetGponPassword( unsigned char *pGponPassword);
extern int kerSysGetSdramSize( void );
extern void kerSysGetBootline(char *string, int strLen);
extern void kerSysSetBootline(char *string, int strLen);
extern void kerSysMipsSoftReset(void);
extern void kerSysLedCtrl(BOARD_LED_NAME, BOARD_LED_STATE);
extern int kerSysFlashSizeGet(void);
extern int kerSysMemoryMappedFlashSizeGet(void);
extern unsigned long kerSysReadFromFlash( void *toaddr, unsigned long fromaddr, unsigned long len );
extern int kerSysEraseFlash(unsigned long eraseaddr, unsigned long len);
extern int kerSysWriteToFlash( unsigned long toaddr, void * fromaddr, unsigned long len);
extern void kerSysRegisterDyingGaspHandler(char *devname, void *cbfn, void *context);
extern void kerSysDeregisterDyingGaspHandler(char *devname);
extern int kerSysIsDyingGaspTriggered(void);
extern int kerConfigCs(BOARD_IOCTL_PARMS *parms);

extern int kerSysGetPciePortEnable(int port);
extern int kerSysGetUsbHostPortEnable(int port);
extern int kerSysGetUsbDeviceEnable(void);
extern int kerSysGetUsb30HostEnable(void);
extern int kerSysSetUsbPower(int on, USB_FUNCTION func);
extern void kerSysSetBootParm(char *name, char *value);

extern void kerSysInitPinmuxInterface(unsigned long interface);

#if defined(WIRELESS)
void kerSysSesEventTrigger( int forced );
#endif


#if defined(GPL_CODE)
extern int AEI_getCurrentUpWanType(void);
#endif


#define  WL_SROM_DEFAULT_FILE           "/mnt/mfg_data/.wlsromcustomerfile.nvm"
#define  WL_SROM_CUSTOMER_FILE          "/data/.wlsromcustomerfile.nvm"

extern unsigned char kerSysGetWlanFeature(void);

/* private functions used within board driver only */
void stopOtherCpu(void);
void resetPwrmgmtDdrMips(void);

PBP_MOCA_INFO boardGetMocaInfo(int dev);

/*
 * Access to shared GPIO registers should be done by calling these
 * functions below, which will grab a spinlock while accessing the
 * GPIO register.  However, if your code needs to access a shared
 * GPIO register but cannot call these functions, you should still
 * acquire the spinlock.
 * (Technically speaking, I should include spinlock.h and declare extern here
 * but that breaks the C++ compile of xtm.)
 */
//#include <linux/spinlock.h>
//extern spinlock_t bcm_gpio_spinlock;
extern void kerSysSetGpioState(unsigned short bpGpio, GPIO_STATE_t state);
extern void kerSysSetGpioStateLocked(unsigned short bpGpio, GPIO_STATE_t state);
extern void kerSysSetGpioDir(unsigned short bpGpio);
extern void kerSysSetGpioDirLocked(unsigned short bpGpio);
extern int kerSysSetGpioDirInput(unsigned short bpGpio);
extern int kerSysGetGpioValue(unsigned short bpGpio);


// for the voice code, which has too many kernSysSetGpio to change
#define kerSysSetGpio kerSysSetGpioState


extern unsigned long kerSysGetUbusFreq(unsigned long miscStrapBus);
extern int kerSysGetAfeId( unsigned long *afeId );
#define __kerSysGetAfeId  kerSysGetAfeId

typedef void (*MocaHostIntrCallback)(int irq, void * arg);
extern void kerSysMocaHostIntrReset(int dev);
extern void kerSysRegisterMocaHostIntrCallback(MocaHostIntrCallback callback, void * userArg, int dev);
extern void kerSysMocaHostIntrEnable(int dev);
extern void kerSysMocaHostIntrDisable(int dev);

extern unsigned int kerSysGetExtIntInfo(unsigned int irq);

void kerSysRegisterPlcUkeCallback( void (* callback)(unsigned long timeInMs, unsigned long param), unsigned long param );
void kerSysDeregisterPlcUkeCallback( void (* callback)(unsigned long timeInMs, unsigned long param) );
void kerSysRegisterPlcRandomizeCallback( void (* callback)(unsigned long timeInMs, unsigned long param), unsigned long param );
void kerSysDeregisterPlcRandomizeCallback( void (* callback)(unsigned long timeInMs, unsigned long param) );

/* implement in arch dependent setup.c */
extern void* kerSysGetDslPhyMemory(void);

#if defined(CONFIG_BCM_CPLD1)
int BcmCpld1Initialize(void);
void BcmCpld1CheckShutdownMode(void);
void BcmCpld1SetShutdownMode(void);
void BcmCpld1SetStandbyTimer(unsigned int duration);
#endif

#if defined (CONFIG_BCM_AVS_PWRSAVE)
extern void kerSysBcmEnableAvs(int enable);
extern int kerSysBcmAvsEnabled(void);
#endif

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
void kerSysRegisterWatchdogCB(char *devname, void *cbfn, void *context);
void kerSysDeregisterWatchdogCB(char *devname);
int bcm_suspend_watchdog(void);
void bcm_resume_watchdog(void);
#endif

#if defined(CONFIG_BCM_DDR_SELF_REFRESH_PWRSAVE)
#define CONFIG_BCM_PWRMNGT_DDR_SR_API
// The structure below is to be declared in ADSL PHY MIPS LMEM, if ADSL is compiled in
typedef struct _PWRMNGT_DDR_SR_CTRL_ {
  union {
   struct {
      unsigned char   phyBusy;
      unsigned char   tp0Busy;
      unsigned char   tp1Busy;
      unsigned char   reserved;
    };
    unsigned int      word;
  };
} PWRMNGT_DDR_SR_CTRL;

void BcmPwrMngtRegisterLmemAddr(PWRMNGT_DDR_SR_CTRL *pDdrSr);
#endif

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
#define TM_BASE_ADDR_STR				"tm"
#define TM_MC_BASE_ADDR_STR				"mc"
#define DHD_BASE_ADDR_STR				"dhd0"
#define DHD_BASE_ADDR_STR_1				"dhd1"
#define DHD_BASE_ADDR_STR_2				"dhd2"
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define TM_DEF_DDR_SIZE 				0x1400000
#else
#define TM_DEF_DDR_SIZE 				0xF00000
#endif
#define TM_ERASED_NVRAM_DEF_DDR_SIZE    0x14
#define TM_MC_DEF_DDR_SIZE				0x0400000
#define DHD_DEF_DDR_SIZE 				0x1400000
#define RDPA_WAN_TYPE_PSP_KEY           "RdpaWanType"
#define RDPA_WAN_OEMAC_PSP_KEY          "WanOEMac"
#define RDPA_EPON_SPEED_PSP_KEY         "EponSpeed"
#define PSP_BUFLEN_16                    16
int BcmMemReserveGetByName(char *name, void **addr, unsigned int *size);
extern int rdp_post_init(void);
extern int rdp_post_init_fiber(void);
#endif

#endif

#ifdef __cplusplus
}
#endif

/* ACTIONTEC Specific defines. */
#if defined(GPL_CODE_WAN_ETH)
#define IF_NAME_EWAN    "ewan"
#endif
#if defined(GPL_CODE)
int kerClearScratchPad(int blk_size);
extern int kerSysGetDslDatapump(unsigned char *dslDatapump);
extern void restoreDatapump(int value);
#endif
#ifdef GPL_CODE_TWO_IN_ONE_FIRMWARE
extern int kerSysGetBoardID(unsigned char *boardid);
#endif
#if defined(GPL_CODE_CONFIG_JFFS)
extern int AEI_kerSysBcmImageSet( int partition, char *string, int img_size );
#endif

#if defined(GPL_CODE_CHIP_D0_CHECK)
extern int  CPURevId;
extern char CPUSerialNumber[32];
#endif

#ifdef GPL_CODE_NAND_CNT_128K
int AEI_kerSysGetFlashCNT(char  *buf,char *type);
int AEI_kerSysGetFlashCNTBufSize( char *len,char *type);
#endif

#if defined(BUILD_AEI_QUANTENNA_LIB)
#define AEI_QTN_ETH_INDEX     5
#define AEI_QTN_BSSID_INDEX   6
#endif
#if defined(GPL_CODE) && (defined(CONFIG_BCM963138) /*|| defined(CONFIG_BCM963268)*/)
void set_bootstatus(unsigned int status);
#endif
#endif /* _BOARD_H */

