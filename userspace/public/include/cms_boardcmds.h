/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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


#ifndef __CMS_BOARDCMDS_H__
#define __CMS_BOARDCMDS_H__

#include <sys/ioctl.h>
#include "cms.h"

/*!\file cms_boardcmds.h
 * \brief Header file for the Board Control Command API.
 *
 * These functions are the simple board control functions that other apps,
 * including GPL apps, may need.  These functions are mostly just wrappers
 * around devCtl_boardIoctl().  The nice things about this file is that
 * it does not require the program to link against additional bcm kernel
 * driver header files.
 *
 */


/** Get the board's br0 interface mac address.
 * 
 * @param macAddrNum (OUT) The user must pass in an array of UINT8 of at least
 *                         MAC_ADDR_LEN (6) bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet devCtl_getBaseMacAddress(UINT8 *macAddrNum);


/** Get the available interface mac address.
 * 
 * @param macAddrNum (OUT) The user must pass in an array of UINT8 of at least
 *                         MAC_ADDR_LEN (6) bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet devCtl_getMacAddress(UINT8 *macAddrNum, UINT32 ulId);


/** Get the given number of consecutive available mac addresses.
 * 
 * @param macAddrNum (OUT) The user must pass in an array of UINT8 of at least
 *                         MAC_ADDR_LEN (6) bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet devCtl_getMacAddresses(UINT8 *macAddrNum, UINT32 ulId, UINT32 num_addresses);

/** Releases the given number of consecutive mac addresses
 * 
 * @param macAddrNum (OUT) The user must pass in an array of UINT8 of at least
 *                         MAC_ADDR_LEN (6) bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet devCtl_releaseMacAddresses(UINT8 *macAddrNum, UINT32 num_addresses);

/** Release the interface mac address that is not used anymore
 * 
 * @param macAddrNum (OUT) The user must pass in an array of UINT8 of at least
 *                         MAC_ADDR_LEN (6) bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet devCtl_releaseMacAddress(UINT8 *macAddrNum);


#if defined(GPL_CODE_FACTORY_TEST)

/** Get the BoadId Number
 *
 * @param BoadIdNumber (OUT) The user must pass in an array of UINT8 of at least
 *                         NVRAM_BOARD_ID_STRING_LEN(16) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getBoardIDNumber(UINT8 *BoadIDNumber);
/** Get the Serial Number
 *
 * @param SerialNumber (OUT) The user must pass in an array of UINT8 of at least
 *                         (33) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getBoardSerialNumber(UINT8 *SerialNumber);

/** Get the HW Version
 *
 * @param HwVersion (OUT) The user must pass in an array of UINT8 of at least
 *                         (9) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getHwVersion(UINT8 *HwVersion);

/** Get the Voice BoadId
 *
 * @param BoadId (OUT) The user must pass in an array of UINT8 of at least
 *                         NVRAM_BOARD_ID_STRING_LEN(16) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getVoiceBoardIDNumber(UINT8 *BoadID);

/** Get the WPA Key
 *
 * @param WPAKey (OUT) The user must pass in an array of UINT8 of at least
 *                         (65) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getWPAKey(UINT8 *WPAKey);

/** Get the Wps AP Pin
 *
 * @param WpsApPin (OUT) The user must pass in an array of UINT8 of at least
 *                         (9) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getWpsAPPin(UINT8 *WpsApPin);

/** Get the Admin password
 *
 * @param Password (OUT) The user must pass in an array of UINT8 of at least
 *                         (33) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getAdminPassword(UINT8 *Password);

/** Get the Burn in Test status
 *
 * @param BurnInTest (OUT) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getBurnInTest(UINT8 *BurnInTest);

/** Set the Burn in Test status
 *
 * @param BurnInTest (IN) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setBurnInTest(UINT8 *BurnInTest);

/** Set the RMA Test status
 *
 * @param BurnInTest (IN) The user must pass in an array of UINT8 of at least
 *                          (2) bytes long.
 *
 *
 * @return CmsRet enum.
 */
CmsRet setRMATest(UINT8 *RMATest);
/** Get the Manu Mode status
 *
 * @param ManuMode (OUT) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getManuMode(UINT8 *ManuMode);

/** Set the Manu Mode status
 *
 * @param ManuMode (IN) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setManuMode(UINT8 *ManuMode);
#ifdef GPL_CODE_DETECT_BOARD_ID
/** Get the auto detect board id flag
 *
 * @param AutoDetectBID (IN) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getAutoDetectBID(UINT8 *data);

/** Set the auto detect board id flag
 *
 * @param AutoDetectBID (IN) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setAutoDetectBID(UINT8 *data);
/** Get the hw board id
 *
 * @param HwBoardId(IN) The user must pass in an array of UINT8 of at least
 *                         (2) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getHwBoardId(UINT8 *data);
#endif

/** Get the RDP TM size
 *
 * @param size (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getRdpTmSize(UINT8 *size);

/** Set the RDP TM size
 *
 * @param size (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setRdpTmSize(UINT8 *size);

/** Get the RDP MC size
 *
 * @param size (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getRdpMcSize(UINT8 *size);

/** Set the RDP MC size
 *
 * @param size (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setRdpMcSize(UINT8 *size);

/** Get the DHD MEM 0 size
 *
 * @param size (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getDhdMem0Size(UINT8 *size);

/** Set the DHD MEM 0 size
 *
 * @param size (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setDhdMem0Size(UINT8 *size);

/** Get the DHD MEM 1 size
 *
 * @param size (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getDhdMem1Size(UINT8 *size);

/** Set the DHD MEM 1 size
 *
 * @param size (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setDhdMem1Size(UINT8 *size);

/** Get the DHD MEM 2 size
 *
 * @param size (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getDhdMem2Size(UINT8 *size);

/** Set the DHD MEM 2 size
 *
 * @param size (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setDhdMem2Size(UINT8 *size);

/** set WPA KEY value
 *
 * @param WPA KEY (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setWPAKey(UINT8 *WPAKey);

/** set the Wps AP Pin
 *
 * @param WpsApPin (IN) The user must pass in an array of UINT8
 *                         (8) bytes long.
 *
 * @return CmsRet enum.
*/
CmsRet setWPSApPin(UINT8 *Wpspin);

/** set the admin password
 *
 * @param password (IN) The user must pass in an array of UINT8
 *                         (32) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setAdminPassword(UINT8 *password);

/** set the BoardSn
 *
 * @param BoardSn (IN) The user must pass in an array of UINT8
 *                         (32) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setBoardSN(UINT8 *BoardSn);

/** set the HwVersion
 *
 * @param HwVersion (IN) The user must pass in an array of UINT8
 *                         (8) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet SetHwVersion(UINT8 *HwVersion);

/** set the mac address number
 *
 * @param mac address number (IN) The user must pass in an array of UINT8
 *                         (4) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setNumberMACAddresses(UINT8 *number);

/** Get the factory firmware version
 *
 * @param FwVersion (OUT) The user must pass in an array of UINT8 of at least
 *                         (48) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getFactoryFmVersion(UINT8 *FwVersion);

/** set the factory firmware version
 *
 * @param factory firmware version (IN) The user must pass in an array of UINT8
 *                         (48) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet setFactoryFmVersion(UINT8 *FwVersion);

/** get the base mac address 
 *
 * @param base mac address number (OUT) The user must pass in an array of UINT8
 *                         (6) bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet getBaseMacAddress(UINT8 *macAddress);

/** set the base mac address 
 *
 * @param base mac address number (IN) The user must pass in an array of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet SetBaseMacAddress(UINT8 *macaddress);

/** Get the Backup PSI Status
 *
 * @param EnableBackupPSI (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getEnableBackupPSI(UINT8 *EnableBackupPSI);

/** Set the Backup PSI Status
 *
 * @param EnableBackupPSI (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setEnableBackupPSI(UINT8 *EnableBackupPSI);

/** Get the System Log Size
 *
 * @param SystemLogSize (OUT) The user must pass in an point of UINT32
 *
 * @return CmsRet enum.
 */
CmsRet getSystemLogSize(UINT32 *SystemLogSize);

/** Set the System Log Size
 *
 * @param SystemLogSize (IN) The user must pass in an point of UNIT8
 *
 * @return CmsRet enum.
 */
CmsRet setSystemLogSize(UINT8 *SystemLogSize);

/** Get the Psi Size
 *
 * @param getPsiSize (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getPsiSize(UINT8 * psiSize);

/** Set the Psi Size
 *
 * @param setPsiSize (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setPsiSize(UINT8 *psiSize);

/** Get the Auxillary File System Size
 *
 * @param AuxillaryFileSystemSize(OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getAuxillaryFileSystemSize(UINT8 *AuxillaryFileSystemSize);
/** Get the Main Thread Number
 *
 * @param MainThreadNumber (OUT) The user must pass in an point of UINT32
 *
 * @return CmsRet enum.
 */
CmsRet getMainThreadNumber(UINT32 *MainThreadNumber);
/** Get the Number MAC Addresses
 *
 * @param NumberMACAddresses (OUT) The user must pass in an point of UINT32
 *
 * @return CmsRet enum.
 */
CmsRet getNumberMACAddresses(UINT32 *NumberMACAddresses);
/** Get the WLAN Feature
 *
 * @param WLANFeature (OUT) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet getWLANFeature(UINT8 *WLANFeature);
/** Set the WLAN Feature
 *
 * @param WLANFeature (IN) The user must pass in an point of UINT8
 *
 * @return CmsRet enum.
 */
CmsRet setWLANFeature(UINT8 *WLANFeature);
#endif

/** Get the number of ethernet MACS on the system.
 * 
 * @return number of ethernet MACS.
 */
UINT32 devCtl_getNumEnetMacs(void);


/** Get the number of ethernet ports on the system.
 * 
 * @return number of ethernet ports.
 */
UINT32 devCtl_getNumEnetPorts(void);


/** Get SDRAM size on the system.
 * 
 * @return SDRAM size in number of bytes.
 */
UINT32 devCtl_getSdramSize(void);


/** Get the chipId.
 *
 * This info is used in various places, including CLI and writing new
 * flash image.  It may be accessed by GPL apps, so it cannot be put
 * exclusively in the data model.
 *  
 * @param chipId (OUT) The chip id returned by the kernel.
 * @return CmsRet enum.
 */
CmsRet devCtl_getChipId(UINT32 *chipId);


/** Set the boot image state.
 *
 * @param state (IN)   BOOT_SET_NEW_IMAGE, BOOT_SET_OLD_IMAGE,
 *                     BOOT_SET_NEW_IMAGE_ONCE,
 *                     BOOT_SET_PART1_IMAGE, BOOT_SET_PART2_IMAGE,
 *                     BOOT_SET_PART1_IMAGE_ONCE, BOOT_SET_PART2_IMAGE_ONCE
 *
 * @return CmsRet enum.
 */
CmsRet devCtl_setImageState(int state);

/** Get the boot image state.
 *
 * @return             BOOT_SET_PART1_IMAGE, BOOT_SET_PART2_IMAGE,
 *                     BOOT_SET_PART1_IMAGE_ONCE, BOOT_SET_PART2_IMAGE_ONCE
 *
 */
int devCtl_getImageState(void);


/** Get image version string.
 *
 * @return number of bytes copied into verStr
 */
int devCtl_getImageVersion(int partition, char *verStr, int verStrSize);
 
 
/** Get the booted image partition.
 *
 * @return             BOOTED_PART1_IMAGE, BOOTED_PART2_IMAGE
 */
int devCtl_getBootedImagePartition(void);


/** Get the booted image id.
 *
 * @return             BOOTED_NEW_IMAGE, BOOTED_OLD_IMAGE
 */
int devCtl_getBootedImageId(void);

#if defined(EPON_SDK_BUILD)
/** Get mac type for a specified uni port
 *
 *  @return success or failure.
 */

UINT32 devCtl_getPortMacType(unsigned short port, unsigned int *mac_type);
    
/** Get the number of FE ports on the system.
 * 
 * @return success or failure.
 */
UINT32 devCtl_getNumFePorts(unsigned int *fe_ports);

/** Get the number of GE ports on the system.
 * 
 * @return success or failure.
 */
UINT32 devCtl_getNumGePorts(unsigned int *ge_ports);

/** Get the number of VoIP ports on the system.
 * 
 * @return success or failure.
 */
UINT32 devCtl_getNumVoipPorts(unsigned int *voip_ports);

/** Get the port_map of internal switch ports used.
 * 
 * @return success or failure.
 */
UINT32 devCtl_getPortMap(unsigned int *port_map);

#endif

#endif /* __CMS_BOARDCMDS_H__ */
