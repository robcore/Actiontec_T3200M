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
 * :>
 ************************************************************************/


#ifndef __IMAGE_NAND_H__
#define __IMAGE_NAND_H__

#define FLASH_INFO_FLAG_NOR    0x0001
#define FLASH_INFO_FLAG_NAND   0x0002

/** Get info about the flash.  Currently, just returns the type of flash,
 *  but in the future, could return more useful info.
 *
 *  @flags (OUT)  Bit field containing info about the flash type.
 *
 *  @return CmsRet enum.
 */

int writeImageToNand( char *string, int size );
int getFlashInfo(unsigned int *flags);

#if defined(GPL_CODE_CONFIG_JFFS)
int writeDualImageToNand( char *string, int size, int partition );
int getDualImageVerFromNand( char *string, int size, int imageNumber );
int getDualImageProductidFromNand( char *string, int size, int imageNumber );
unsigned int AEI_getSequenceNumber(int imageNumber);
unsigned int AEI_getFlashFreeSize();
#endif
#endif /*__CMS_IMAGE_H__*/

