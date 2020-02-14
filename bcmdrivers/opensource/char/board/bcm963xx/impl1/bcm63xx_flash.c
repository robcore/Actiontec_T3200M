
/*
<:copyright-BRCM:2013:DUAL/GPL:standard 

   Copyright (c) 2013 Broadcom Corporation
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
/*
 ***************************************************************************
 * File Name  : bcm63xx_flash.c
 *
 * Description: This file contains the flash device driver APIs for bcm63xx board. 
 *
 * Created on :  8/10/2002  seanl:  use cfiflash.c, cfliflash.h (AMD specific)
 *
 ***************************************************************************/


/* Includes. */
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/preempt.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/jffs2.h>
#include <linux/mount.h>
#include <linux/crc32.h>
#include <linux/sched.h>
#include <linux/bcm_assert_locks.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/compiler.h>

#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <board.h>
#include <bcmTag.h>
#include "flash_api.h"
#include "boardparms.h"
#include "boardparms_voice.h"

#ifdef GPL_CODE_NAND_CNT_128K
#include <linux/mtd/partitions.h>
#endif

#if defined(CONFIG_BCM968500)
#include "bl_lilac_mips_blocks.h"
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#include "pmc_drv.h"
#endif

#include <linux/fs_struct.h>
//#define DEBUG_FLASH

#ifdef GPL_CODE_NAND_IMG_CHECK
extern int gSetWrongCRC; //1=set wrong crc
#endif

#ifdef GPL_CODE
extern void aei_mtd_lock(void);
extern void aei_mtd_unlock(void);
//#define BCMTAG_EXE_USE
static unsigned long Crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};
static UINT32 getCrc32(byte *pdata, UINT32 size, UINT32 crc)
{
    while (size-- > 0)
        crc = (crc >> 8) ^ Crc32_table[(crc ^ *pdata++) & 0xff];

    return crc;
}
#endif

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
int otp_is_btrm_boot(void);
int otp_is_boot_secure(void);
#endif

extern int kerSysGetSequenceNumber(int);
extern PFILE_TAG kerSysUpdateTagSequenceNumber(int);

/*
 * inMemNvramData an in-memory copy of the nvram data that is in the flash.
 * This in-memory copy is used by NAND.  It is also used by NOR flash code
 * because it does not require a mutex or calls to memory allocation functions
 * which may sleep.  It is kept in sync with the flash copy by
 * updateInMemNvramData.
 */
static unsigned char *inMemNvramData_buf;
static NVRAM_DATA inMemNvramData;
static DEFINE_SPINLOCK(inMemNvramData_spinlock);
static void updateInMemNvramData(const unsigned char *data, int len, int offset);
#define UNINITIALIZED_FLASH_DATA_CHAR  0xff
static FLASH_ADDR_INFO fInfo;
static struct semaphore semflash;
#ifdef GPL_CODE
static struct semaphore rd_semflash;
#endif

// mutex is preferred over semaphore to provide simple mutual exclusion
// spMutex protects scratch pad writes
static DEFINE_MUTEX(spMutex);
extern struct mutex flashImageMutex;
static int bootFromNand = 0;

static int setScratchPad(char *buf, int len);
static char *getScratchPad(int len);
static int nandNvramSet(const char *nvramString );

// Variables not used in the simplified API used for the IKOS target
#if !defined(CONFIG_BRCM_IKOS)
static char bootCfeVersion[CFE_VERSION_MARK_SIZE+CFE_VERSION_SIZE];
#endif

#ifdef GPL_CODE_CONFIG_JFFS
static int nandEraseBlk( struct mtd_info *mtd, int blk );
#endif

#define ALLOC_TYPE_KMALLOC   0
#define ALLOC_TYPE_VMALLOC   1

static void *retriedKmalloc(size_t size)
{
    void *pBuf;
    unsigned char *bufp8 ;

    size += 4 ; /* 4 bytes are used to store the housekeeping information used for freeing */

    // Memory allocation changed from kmalloc() to vmalloc() as the latter is not susceptible to memory fragmentation under low memory conditions
    // We have modified Linux VM to search all pages by default, it is no longer necessary to retry here
    if (!in_interrupt() ) {
        pBuf = vmalloc(size);
        if (pBuf) {
            memset(pBuf, 0, size);
            bufp8 = (unsigned char *) pBuf ;
            *bufp8 = ALLOC_TYPE_VMALLOC ;
            pBuf = bufp8 + 4 ;
        }
    }
    else { // kmalloc is still needed if in interrupt
        pBuf = kmalloc(size, GFP_ATOMIC);
        if (pBuf) {
            memset(pBuf, 0, size);
            bufp8 = (unsigned char *) pBuf ;
            *bufp8 = ALLOC_TYPE_KMALLOC ;
            pBuf = bufp8 + 4 ;
        }
    }

    return pBuf;
}

static void retriedKfree(void *pBuf)
{
    unsigned char *bufp8  = (unsigned char *) pBuf ;
    bufp8 -= 4 ;

    if (*bufp8 == ALLOC_TYPE_KMALLOC)
        kfree(bufp8);
    else
        vfree(bufp8);
}

// get shared blks into *** pTempBuf *** which has to be released bye the caller!
// return: if pTempBuf != NULL, poits to the data with the dataSize of the buffer
// !NULL -- ok
// NULL  -- fail
static char *getSharedBlks(int start_blk, int num_blks)
{
    int i = 0;
    int usedBlkSize = 0;
    int sect_size = 0;
    char *pTempBuf = NULL;
    char *pBuf = NULL;

    down(&semflash);

    for (i = start_blk; i < (start_blk + num_blks); i++)
        usedBlkSize += flash_get_sector_size((unsigned short) i);

    if ((pTempBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
        printk("failed to allocate memory with size: %d\n", usedBlkSize);
        up(&semflash);
        return pTempBuf;
    }
    
    pBuf = pTempBuf;
    for (i = start_blk; i < (start_blk + num_blks); i++)
    {
        sect_size = flash_get_sector_size((unsigned short) i);

#if defined(DEBUG_FLASH)
        printk("getSharedBlks: blk=%d, sect_size=%d\n", i, sect_size);
#endif
        flash_read_buf((unsigned short)i, 0, pBuf, sect_size);
        pBuf += sect_size;
    }
    up(&semflash);
    
    return pTempBuf;
}

// Set the pTempBuf to flash from start_blk for num_blks
// return:
// 0 -- ok
// -1 -- fail
static int setSharedBlks(int start_blk, int num_blks, char *pTempBuf)
{
    int i = 0;
    int sect_size = 0;
    int sts = 0;
    char *pBuf = pTempBuf;

    down(&semflash);

    for (i = start_blk; i < (start_blk + num_blks); i++)
    {
        sect_size = flash_get_sector_size((unsigned short) i);
        flash_sector_erase_int(i);

        if (flash_write_buf(i, 0, pBuf, sect_size) != sect_size)
        {
            printk("Error writing flash sector %d.", i);
            sts = -1;
            break;
        }

#if defined(DEBUG_FLASH)
        printk("setShareBlks: blk=%d, sect_size=%d\n", i, sect_size);
#endif

        pBuf += sect_size;
    }

    up(&semflash);

    return sts;
}

#if !defined(CONFIG_BRCM_IKOS)
// Initialize the flash and fill out the fInfo structure
void kerSysEarlyFlashInit( void )
{
#ifdef GPL_CODE
   unsigned long flags;
   NVRAM_DATA NvramDataTmp;
   unsigned char *pStrTmp=(unsigned char *)&NvramDataTmp;
#endif
    int flash_type;

#ifdef CONFIG_BCM_ASSERTS
    // ASSERTS and bug() may be too unfriendly this early in the bootup
    // sequence, so just check manually
    if (sizeof(NVRAM_DATA) != NVRAM_LENGTH)
        printk("kerSysEarlyFlashInit: nvram size mismatch! "
               "NVRAM_LENGTH=%d sizeof(NVRAM_DATA)=%d\n",
               NVRAM_LENGTH, sizeof(NVRAM_DATA));
#endif
    inMemNvramData_buf = (unsigned char *) &inMemNvramData;
    memset(inMemNvramData_buf, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);

    flash_init();

    flash_type = flash_get_flash_type();
    if ((flash_type == FLASH_IFC_NAND) || (flash_type == FLASH_IFC_SPINAND))
        bootFromNand = 1;
    else
        bootFromNand = 0;


#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM96328) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
    if (flash_type == FLASH_IFC_NAND)
    {
        unsigned int bootCfgSave =  NAND->NandNandBootConfig;

        NAND->NandNandBootConfig = NBC_AUTO_DEV_ID_CFG | 0x101;
        NAND->NandCsNandXor = 1;
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        memcpy((unsigned char *)&bootCfeVersion, (unsigned char *)
            NANDFLASH_BASE + CFE_VERSION_OFFSET, sizeof(bootCfeVersion));
        memcpy(inMemNvramData_buf, (unsigned char *)
            NANDFLASH_BASE + NVRAM_DATA_OFFSET, sizeof(NVRAM_DATA));
#else
        memcpy((unsigned char *)&bootCfeVersion, (unsigned char *)
            FLASH_BASE + CFE_VERSION_OFFSET, sizeof(bootCfeVersion));
        memcpy(inMemNvramData_buf, (unsigned char *)
            FLASH_BASE + NVRAM_DATA_OFFSET, sizeof(NVRAM_DATA));
#endif
        NAND->NandNandBootConfig = bootCfgSave;
        NAND->NandCsNandXor = 0;
    }
    else
#endif
#if defined (CONFIG_MTD_BCM_SPI_NAND)
    if (flash_type == FLASH_IFC_SPINAND)
    {
        /* Load boot board parameters saved by function bootNandImageFromRootfs
         * in file bcm63xx_ram\bcm63xx_cmd.c, bypassing requirement for early driver support
         */
        kerSysBlParmsGetStr(BOARD_ID_NAME, inMemNvramData.szBoardId, NVRAM_BOARD_ID_STRING_LEN);
        kerSysBlParmsGetStr(VOICE_BOARD_ID_NAME, inMemNvramData.szVoiceBoardId, NVRAM_BOARD_ID_STRING_LEN);
        kerSysBlParmsGetInt(BOARD_STUFF_NAME, &inMemNvramData.ulBoardStuffOption);
    }
    else
#endif
    {
        fInfo.flash_rootfs_start_offset = flash_get_sector_size(0);
#if defined (CONFIG_BCM96838)
        if( fInfo.flash_rootfs_start_offset < PSRAM_SIZE )
            fInfo.flash_rootfs_start_offset = PSRAM_SIZE;
#elif defined(CONFIG_BCM96848)
        fInfo.flash_rootfs_start_offset = 192*1024;
#else
        if( fInfo.flash_rootfs_start_offset < FLASH_LENGTH_BOOT_ROM )
        {
#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
            if (otp_is_btrm_boot())
            fInfo.flash_rootfs_start_offset = FLASH_LENGTH_SECURE_BOOT_ROM;
            else
#endif
            fInfo.flash_rootfs_start_offset = FLASH_LENGTH_BOOT_ROM;
        }
#endif
        fInfo.flash_rootfs_start_offset += IMAGE_OFFSET;

        flash_read_buf (NVRAM_SECTOR, CFE_VERSION_OFFSET,
            (unsigned char *)&bootCfeVersion, sizeof(bootCfeVersion));

        /* Read the flash contents into NVRAM buffer */
        flash_read_buf (NVRAM_SECTOR, NVRAM_DATA_OFFSET,
                        inMemNvramData_buf, sizeof (NVRAM_DATA)) ;
    }
#ifdef GPL_CODE
    /* Enable Backup PSI by default */
    //printk("###set backup psi before,crc=%x\n",inMemNvramData.ulCheckSum);
    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    if(inMemNvramData.backupPsi != 0x01)
    {
        inMemNvramData.backupPsi = 0x01;

        inMemNvramData.ulCheckSum=0;
        inMemNvramData.ulCheckSum = getCrc32(inMemNvramData_buf, sizeof (NVRAM_DATA), CRC32_INIT_VALUE);
        memset(pStrTmp, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);
        memcpy(pStrTmp,inMemNvramData_buf, sizeof(NVRAM_DATA));
        //printk("###set backup psi end,crc=%x\n",inMemNvramData.ulCheckSum);
        //printk("Enable Backup PSI inMemNvramData.backupPsi = 0x%x\n", inMemNvramData.backupPsi);
    }
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
#endif  // GPL_CODE

#if defined(DEBUG_FLASH)
    printk("reading nvram into inMemNvramData\n");
    printk("ulPsiSize 0x%x\n", (unsigned int)inMemNvramData.ulPsiSize);
    printk("backupPsi 0x%x\n", (unsigned int)inMemNvramData.backupPsi);
    printk("ulSyslogSize 0x%x\n", (unsigned int)inMemNvramData.ulSyslogSize);
#endif

    if ((BpSetBoardId(inMemNvramData.szBoardId) != BP_SUCCESS))
        printk("\n*** Board is not initialized properly ***\n\n");
#if defined (CONFIG_BCM_ENDPOINT_MODULE)
    if ((BpSetVoiceBoardId(inMemNvramData.szVoiceBoardId) != BP_SUCCESS))
        printk("\n*** Voice Board id is not initialized properly ***\n\n");

    if(BpGetVoiceDectType(inMemNvramData.szBoardId) != BP_VOICE_NO_DECT) 
     {
       BpSetDectPopulatedData((int)(inMemNvramData.ulBoardStuffOption & VOICE_OPTION_DECT_MASK));
     }
#endif

}

/***********************************************************************
 * Function Name: kerSysCfeVersionGet
 * Description  : Get CFE Version.
 * Returns      : 1 -- ok, 0 -- fail
 ***********************************************************************/
int kerSysCfeVersionGet(char *string, int stringLength)
{
    memcpy(string, (unsigned char *)&bootCfeVersion, stringLength);
    return(0);
}

/****************************************************************************
 * NVRAM functions
 * NVRAM data could share a sector with the CFE.  So a write to NVRAM
 * data is actually a read/modify/write operation on a sector.  Protected
 * by a higher level mutex, flashImageMutex.
 * Nvram data is cached in memory in a variable called inMemNvramData, so
 * writes will update this variable and reads just read from this variable.
 ****************************************************************************/


/** set nvram data
 * Must be called with flashImageMutex held
 *
 * @return 0 on success, -1 on failure.
 */
int kerSysNvRamSet(const char *string, int strLen, int offset)
{
    int sts = -1;  // initialize to failure
    char *pBuf = NULL;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
    BCM_ASSERT_R(offset+strLen <= NVRAM_LENGTH, sts);

    if (bootFromNand == 0)
    {
        if ((pBuf = getSharedBlks(NVRAM_SECTOR, 1)) == NULL)
            return sts;

        // set string to the memory buffer
        memcpy((pBuf + NVRAM_DATA_OFFSET + offset), string, strLen);

        sts = setSharedBlks(NVRAM_SECTOR, 1, pBuf);

        retriedKfree(pBuf);       
    }
    else
    {

#if 0
//#ifdef GPL_CODE_CONFIG_JFFS
//Since NAND flash Block 0 is always reliable, it is unneccesary to add the check of reading/writing Block 0.
       struct mtd_info *mtd1 = get_mtd_device_nm("nvram");
       char *tempStorage;
       char *block_buf=NULL;
       int retlen = 0;
       if (NULL != (tempStorage = kmalloc(mtd1->erasesize, GFP_KERNEL)) && mtd1 !=NULL)
       {
            PNVRAM_DATA pnd = NULL;
            unsigned long flags;
            int j=0;
            down(&semflash);
            memset(tempStorage, UNINITIALIZED_FLASH_DATA_CHAR, mtd1->erasesize );
            for(j=0;j<5;j++)
            {
                mtd1->_read(mtd1, 0, mtd1->erasesize, &retlen, tempStorage);
                pnd = (PNVRAM_DATA) (tempStorage + NVRAM_DATA_OFFSET);
                printk("before change szboard(%s) psk(%s) datapump(%ld),offset(%d),len(%d)\n",pnd->szBoardId,pnd->wpaKey,pnd->dslDatapump,offset,strLen);
                if(tempStorage[0]== 0x10 && tempStorage[1] ==0x00 || tempStorage[2]==0x02 || tempStorage[3] == 0x7b) // It is unfit for 63138 platform
                {
                    break;
                }
            }

            if(j==5)
            {
                if( mtd1 )
                    put_mtd_device(mtd1);

                kfree(tempStorage);
                up(&semflash);
                return sts;

            }


            memcpy((tempStorage + NVRAM_DATA_OFFSET + offset), string, strLen);
                        /* Flash the CFE ROM boot loader. */
            nandEraseBlk( mtd1, 0 );
            mtd1->_write(mtd1, 0, mtd1->erasesize, &retlen, tempStorage);

            do
            {
                if (NULL !=(block_buf = kmalloc(mtd1->erasesize, GFP_KERNEL)) )
                {
                    memset(block_buf,0,mtd1->erasesize);
                    mtd1->_read(mtd1, 0, mtd1->erasesize, &retlen, block_buf);
                    if(memcmp(block_buf,tempStorage,mtd1->erasesize)!=0)
                    {
                        printk("##write nvram error\n");
                        nandEraseBlk( mtd1, 0 );
                        mtd1->_write(mtd1, 0, mtd1->erasesize, &retlen, tempStorage);
                        kfree(block_buf);
                    }
                    else
                    {
                        printk("##write nvram correct\n");
                        kfree(block_buf);
                        break;
                    }
                }
                else
                {
                    printk("##malloc fail\n");
                    break;
                }
            }
            while(1);


            up(&semflash);
            spin_lock_irqsave(&inMemNvramData_spinlock, flags);
            memcpy(inMemNvramData_buf + offset,string, strLen);
            spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

            kfree(tempStorage);
            if( mtd1 )
                put_mtd_device(mtd1);

            sts = 0;

        }
#else
        sts = nandNvramSet(string);
#endif
    }
    
    if (0 == sts)
    {
        // write to flash was OK, now update in-memory copy
        updateInMemNvramData((unsigned char *) string, strLen, offset);
    }

    return sts;
}


/** get nvram data
 *
 * since it reads from in-memory copy of the nvram data, always successful.
 */
void kerSysNvRamGet(char *string, int strLen, int offset)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(string, inMemNvramData_buf + offset, strLen);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

    return;
}

/** load nvram data from mtd device
 */
void kerSysNvRamLoad(void * mtd_ptr)
{
    unsigned long flags;
    int retlen = 0;
    struct mtd_info * mtd = mtd_ptr;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);

    /* Read the whole cfe rom nand block 0 */
    mtd_read(mtd, (NVRAM_SECTOR * mtd->erasesize) + CFE_VERSION_OFFSET,
        sizeof(bootCfeVersion), &retlen, (unsigned char *)&bootCfeVersion);

    mtd_read(mtd, (NVRAM_SECTOR * mtd->erasesize) + NVRAM_DATA_OFFSET,
        sizeof(NVRAM_DATA), &retlen, inMemNvramData_buf);

    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

    return;
}


/** Erase entire nvram area.
 *
 * Currently there are no callers of this function.  THe return value is
 * the opposite of kerSysNvramSet.  Kept this way for compatibility.
 *
 * @return 0 on failure, 1 on success.
 */
int kerSysEraseNvRam(void)
{
    int sts = 1;

    BCM_ASSERT_NOT_HAS_MUTEX_C(&flashImageMutex);

    if (bootFromNand == 0)
    {
        char *tempStorage;
        if (NULL == (tempStorage = kmalloc(NVRAM_LENGTH, GFP_KERNEL)))
        {
            sts = 0;
        }
        else
        {
            // just write the whole buf with '0xff' to the flash
            memset(tempStorage, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);
            mutex_lock(&flashImageMutex);
            if (kerSysNvRamSet(tempStorage, NVRAM_LENGTH, 0) != 0)
                sts = 0;
            mutex_unlock(&flashImageMutex);
            kfree(tempStorage);
        }
    }
    else
    {
        printk("kerSysEraseNvram: not supported when bootFromNand == 1\n");
        sts = 0;
    }

    return sts;
}

#else // CONFIG_BRCM_IKOS
static NVRAM_DATA ikos_nvram_data =
    {
    NVRAM_VERSION_NUMBER,
    "",
    "ikos",
    0,
    DEFAULT_PSI_SIZE,
    11,
    {0x02, 0x10, 0x18, 0x01, 0x00, 0x01},
    0x00, 0x00,
    0x720c9f60
    };

void kerSysEarlyFlashInit( void )
{
    inMemNvramData_buf = (unsigned char *) &inMemNvramData;
    memset(inMemNvramData_buf, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);

    memcpy(inMemNvramData_buf, (unsigned char *)&ikos_nvram_data,
        sizeof (NVRAM_DATA));
    fInfo.flash_scratch_pad_length = 0;
    fInfo.flash_persistent_start_blk = 0;
}

int kerSysCfeVersionGet(char *string, int stringLength)
{
    *string = '\0';
    return(0);
}


void kerSysNvRamGet(char *string, int strLen, int offset)
{
    memcpy(string, (unsigned char *) &ikos_nvram_data, sizeof(NVRAM_DATA));
}

int kerSysNvRamSet(const char *string, int strLen, int offset)
{
    if( bootFromNand )
        nandNvramSet(string);
    return(0);
}

int kerSysEraseNvRam(void)
{
    return(0);
}

unsigned long kerSysReadFromFlash( void *toaddr, unsigned long fromaddr,
    unsigned long len )
{
    return((unsigned long) memcpy((unsigned char *) toaddr, (unsigned char *) fromaddr, len));
}
#endif  // CONFIG_BRCM_IKOS

/** Update the in-Memory copy of the nvram with the given data.
 *
 * @data: pointer to new nvram data
 * @len: number of valid bytes in nvram data
 * @offset: offset of the given data in the nvram data
 */
void updateInMemNvramData(const unsigned char *data, int len, int offset)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(inMemNvramData_buf + offset, data, len);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}


/** Get the bootline string from the NVRAM data.
 * Assumes the caller has the inMemNvramData locked.
 * Special case: this is called from prom.c without acquiring the
 * spinlock.  It is too early in the bootup sequence for spinlocks.
 *
 * @param bootline (OUT) a buffer of NVRAM_BOOTLINE_LEN bytes for the result
 */
void kerSysNvRamGetBootlineLocked(char *bootline)
{
    memcpy(bootline, inMemNvramData.szBootline,
                     sizeof(inMemNvramData.szBootline));
}
EXPORT_SYMBOL(kerSysNvRamGetBootlineLocked);


/** Get the bootline string from the NVRAM data.
 *
 * @param bootline (OUT) a buffer of NVRAM_BOOTLINE_LEN bytes for the result
 */
void kerSysNvRamGetBootline(char *bootline)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    kerSysNvRamGetBootlineLocked(bootline);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}
EXPORT_SYMBOL(kerSysNvRamGetBootline);


/** Get the BoardId string from the NVRAM data.
 * Assumes the caller has the inMemNvramData locked.
 * Special case: this is called from prom_init without acquiring the
 * spinlock.  It is too early in the bootup sequence for spinlocks.
 *
 * @param boardId (OUT) a buffer of NVRAM_BOARD_ID_STRING_LEN
 */
void kerSysNvRamGetBoardIdLocked(char *boardId)
{
    memcpy(boardId, inMemNvramData.szBoardId,
                    sizeof(inMemNvramData.szBoardId));
}
EXPORT_SYMBOL(kerSysNvRamGetBoardIdLocked);


/** Get the BoardId string from the NVRAM data.
 *
 * @param boardId (OUT) a buffer of NVRAM_BOARD_ID_STRING_LEN
 */
void kerSysNvRamGetBoardId(char *boardId)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    kerSysNvRamGetBoardIdLocked(boardId);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}
EXPORT_SYMBOL(kerSysNvRamGetBoardId);


/** Get the base mac addr from the NVRAM data.  This is 6 bytes, not
 * a string.
 *
 * @param baseMacAddr (OUT) a buffer of NVRAM_MAC_ADDRESS_LEN
 */
void kerSysNvRamGetBaseMacAddr(unsigned char *baseMacAddr)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(baseMacAddr, inMemNvramData.ucaBaseMacAddr,
                        sizeof(inMemNvramData.ucaBaseMacAddr));
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
#ifdef GPL_CODE
    printk("###baseMacAddr(%s)\n",baseMacAddr);
#endif
}
EXPORT_SYMBOL(kerSysNvRamGetBaseMacAddr);


/** Get the nvram version from the NVRAM data.
 *
 * @return nvram version number.
 */
unsigned long kerSysNvRamGetVersion(void)
{
    return (inMemNvramData.ulVersion);
}
EXPORT_SYMBOL(kerSysNvRamGetVersion);


void kerSysFlashInit( void )
{
    sema_init(&semflash, 1);
#ifdef GPL_CODE
    sema_init(&rd_semflash, 1);
#endif

    // too early in bootup sequence to acquire spinlock, not needed anyways
    // only the kernel is running at this point
    flash_init_info(&inMemNvramData, &fInfo);
}

/***********************************************************************
 * Function Name: kerSysFlashAddrInfoGet
 * Description  : Fills in a structure with information about the NVRAM
 *                and persistent storage sections of flash memory.  
 *                Fro physmap.c to mount the fs vol.
 * Returns      : None.
 ***********************************************************************/
void kerSysFlashAddrInfoGet(PFLASH_ADDR_INFO pflash_addr_info)
{
    memcpy(pflash_addr_info, &fInfo, sizeof(FLASH_ADDR_INFO));
}

/*******************************************************************************
 * PSI functions
 * PSI is where we store the config file.  There is also a "backup" PSI
 * that stores an extra copy of the PSI.  THe idea is if the power goes out
 * while we are writing the primary PSI, the backup PSI will still have
 * a good copy from the last write.  No additional locking is required at
 * this level.
 *******************************************************************************/
#define PSI_FILE_NAME           "/data/psi"
#define PSI_BACKUP_FILE_NAME    "/data/psibackup"
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
#define PSI_CUSTOMER_FILE_NAME    "/data/psicustomer"
#endif
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
#define AEI_IMAGE1_PSI_FILE_NAME  "/data/image1psi"
#define AEI_IMAGE2_PSI_FILE_NAME  "/data/image2psi"
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
#define PSI_OLD_IMAGE_FILE_NAME    "/data/psiOldImage"
#endif
#define SCRATCH_PAD_FILE_NAME   "/data/scratchpad"


// get psi data
// return:
//  0 - ok
//  -1 - fail
int kerSysPersistentGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if ((pBuf = getSharedBlks(fInfo.flash_persistent_start_blk,
        fInfo.flash_persistent_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + fInfo.flash_persistent_blk_offset + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
int kerSysCustomerPsiGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read customer PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_CUSTOMER_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_CUSTOMER_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if (fInfo.flash_customer_psi_number_blk <= 0)
    {
        printk("No customer psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_customer_psi_start_blk,
                              fInfo.flash_customer_psi_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}
#endif

#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
/***************************************
 * name: kerSysOldImageCfgActive
 * description: get the old firmware config file from special block which just save before firmware upgrade. and cover the persistent psi backup psi with it.
 *
 * input:
 *
 * output:
 *
 * return: 0 -- success, -1 -- failure
 *
 * ***********************************************/
int kerSysOldImageCfgActive(void)
{
    int sts = 0;
    char *string;
    int strLen=fInfo.flash_persistent_length;
    char *pBuf = NULL;
    char *pBuf1 = NULL;
    char *pBuf2 = NULL;

    if((string = (char*) retriedKmalloc(fInfo.flash_persistent_length)) == NULL)
    {
       printk("%s, failed to allocate memory with size: %d\n", __func__, fInfo.flash_persistent_length);
        return -1;
    }
    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read old image config file from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_OLD_IMAGE_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            sts=-1;
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_OLD_IMAGE_FILE_NAME);
            else
            {
                sts=kerSysFsFileSet(PSI_BACKUP_FILE_NAME, string, strLen);
                if(sts==0)
                {
                    //cover persistent PSI
                    sts=kerSysFsFileSet(PSI_FILE_NAME, string, strLen);
                }
            }

            filp_close(fp, NULL);
            set_fs(fs);
        }
        //cover backup PSI
        retriedKfree(string);
        return sts;
    }

    if (fInfo.flash_old_image_cfg_number_blk <= 0)
    {
        printk("No old image config file blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_old_image_cfg_start_blk,
                              fInfo.flash_old_image_cfg_number_blk)) == NULL)
        return -1;

    //set persistent
    if ((pBuf1 = getSharedBlks(fInfo.flash_persistent_start_blk,
        fInfo.flash_persistent_number_blk)) == NULL)
        return -1;

    // set string to the memory buffer
    memcpy((pBuf1 + fInfo.flash_persistent_blk_offset ), pBuf, strLen);

    if (setSharedBlks(fInfo.flash_persistent_start_blk, 
        fInfo.flash_persistent_number_blk, pBuf1) != 0)
        sts = -1;

    //set backup psi
    if(fInfo.flash_backup_psi_number_blk > 0 )
    {
        if ((pBuf2 = getSharedBlks(fInfo.flash_backup_psi_start_blk,
            fInfo.flash_backup_psi_number_blk)) == NULL)
            return -1;
        // set string to the memory buffer
        memcpy(pBuf2 , pBuf, strLen);

        if (setSharedBlks(fInfo.flash_backup_psi_start_blk, 
            fInfo.flash_backup_psi_number_blk, pBuf2) != 0)
            sts = -1;
    }
    
    retriedKfree(pBuf);
    retriedKfree(pBuf1);
    retriedKfree(pBuf2);

    return sts;
}
#endif

int kerSysBackupPsiGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read backup PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_BACKUP_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_BACKUP_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if (fInfo.flash_backup_psi_number_blk <= 0)
    {
        printk("No backup psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_backup_psi_start_blk,
                              fInfo.flash_backup_psi_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

int kerSysFsFileSet(const char *filename, char *buf, int len)
{
    int ret = -1;
    struct file *fp;
    mm_segment_t fs;

    fs = get_fs();
    set_fs(get_ds());

    fp = filp_open(filename, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (!IS_ERR(fp))
    {
        if (fp->f_op && fp->f_op->write)
        {
            fp->f_pos = 0;

            if((int) fp->f_op->write(fp, (void *) buf, len, &fp->f_pos) == len)
            {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)                
                vfs_fsync(fp, 0);
#else
                vfs_fsync(fp, fp->f_path.dentry, 0);
#endif
                ret = 0;
            }
        }

        filp_close(fp, NULL);
    }

    set_fs(fs);

    if (ret != 0)
    {
        printk("Failed to write to '%s'.\n", filename);
    }

    return( ret );
}

// set psi 
// return:
//  0 - ok
//  -1 - fail
int kerSysPersistentSet(char *string, int strLen, int offset)
{
    int sts = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_FILE_NAME, string, strLen);
    }

    if ((pBuf = getSharedBlks(fInfo.flash_persistent_start_blk,
        fInfo.flash_persistent_number_blk)) == NULL)
        return -1;

    // set string to the memory buffer
    memcpy((pBuf + fInfo.flash_persistent_blk_offset + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_persistent_start_blk, 
        fInfo.flash_persistent_number_blk, pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
int AEI_kerSysImagePsiGet(BOARD_IOCTL_ACTION action,char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read customer PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        if(action == AEI_IMAGE1_PSI)
           fp = filp_open(AEI_IMAGE1_PSI_FILE_NAME, O_RDONLY, 0);
        else
           fp = filp_open(AEI_IMAGE2_PSI_FILE_NAME, O_RDONLY, 0);

        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read aei_image_psi\n");

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if(action == AEI_IMAGE1_PSI)
    {

        if (fInfo.flash_image1_psi_number_blk <= 0)
        {
             printk("No image1 psi blks allocated, change it in CFE\n");
             return -1;
        }

        if (fInfo.flash_image1_psi_start_blk == 0)
           return -1;

        if ((pBuf = getSharedBlks(fInfo.flash_image1_psi_start_blk,
                              fInfo.flash_image1_psi_start_blk)) == NULL)
             return -1;
    }
    else
    {
        if (fInfo.flash_image2_psi_number_blk <= 0)
        {
             printk("No image1 psi blks allocated, change it in CFE\n");
             return -1;
        }

        if (fInfo.flash_image2_psi_start_blk == 0)
           return -1;

        if ((pBuf = getSharedBlks(fInfo.flash_image2_psi_start_blk,
                              fInfo.flash_image2_psi_start_blk)) == NULL)
             return -1;

    }
    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}
int AEI_kerSysImagePsiSet(BOARD_IOCTL_ACTION action,char *string, int strLen, int offset)
{
     int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write customer PSI to
         * a file on the NAND flash.
         */
         if(action == AEI_IMAGE1_PSI)
            return kerSysFsFileSet(AEI_IMAGE1_PSI_FILE_NAME, string, strLen);
         else
            return kerSysFsFileSet(AEI_IMAGE2_PSI_FILE_NAME, string, strLen);
    }

    if(action == AEI_IMAGE1_PSI)
    {
        if (fInfo.flash_image1_psi_number_blk <= 0)
        {
           printk("No image1 psi blks allocated, change it in CFE\n");
           return -1;
         }

         if (fInfo.flash_image1_psi_start_blk == 0)
           return -1;
    }
    else
    {
        if (fInfo.flash_image2_psi_number_blk <= 0)
        {
           printk("No image2 psi blks allocated, change it in CFE\n");
           return -1;
         }

         if (fInfo.flash_image2_psi_start_blk == 0)
           return -1;
    }

    /*
     * The customer PSI does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the customer PSI spans.
     */
    if(action == AEI_IMAGE1_PSI)
    {
        for (i=fInfo.flash_image1_psi_start_blk;
           i < (fInfo.flash_image1_psi_start_blk + fInfo.flash_image1_psi_number_blk); i++)
        {
           usedBlkSize += flash_get_sector_size((unsigned short) i);
        }
    }
    else
    {
       for (i=fInfo.flash_image2_psi_start_blk;
           i < (fInfo.flash_image2_psi_start_blk + fInfo.flash_image2_psi_number_blk); i++)
        {
           usedBlkSize += flash_get_sector_size((unsigned short) i);
        }


    }

#ifdef GPL_CODE
    if (strLen + offset > usedBlkSize)
    {
        printk("kerSysCustomerPsiSet strLen = %d, offset = %d, usedBlkSize = %d\n", strLen, offset, usedBlkSize);
        return -1;
    }
#endif
    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);
    if(action == AEI_IMAGE1_PSI)
    {
        if (setSharedBlks(fInfo.flash_image1_psi_start_blk, fInfo.flash_image1_psi_number_blk, 
                      pBuf) != 0)
              sts = -1;
    }
    else
    {
        if (setSharedBlks(fInfo.flash_image2_psi_start_blk, fInfo.flash_image2_psi_number_blk, 
                      pBuf) != 0)
              sts = -1;

    }
    
    retriedKfree(pBuf);

    return sts;
   
}
#endif
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
int kerSysCustomerPsiSet(char *string, int strLen, int offset)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write customer PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_CUSTOMER_FILE_NAME, string, strLen);
    }

    if (fInfo.flash_customer_psi_number_blk <= 0)
    {
        printk("No customer psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    /*
     * The customer PSI does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the customer PSI spans.
     */
    for (i=fInfo.flash_customer_psi_start_blk;
         i < (fInfo.flash_customer_psi_start_blk + fInfo.flash_customer_psi_number_blk); i++)
    {
       usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

#ifdef GPL_CODE
    if (strLen + offset > usedBlkSize)
    {
        printk("kerSysCustomerPsiSet strLen = %d, offset = %d, usedBlkSize = %d\n", strLen, offset, usedBlkSize);
        return -1;
    }
#endif
    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_customer_psi_start_blk, fInfo.flash_customer_psi_number_blk, 
                      pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}
#endif

#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
/***************************************
 * name: kerSysOldImageCfgSet
 * description: save valid input CFG into special block as the old firmware config file just before firmware upgrade. 
 *
 * input:
 * char *string -- the valid input config file
 * int strLen   -- the input config file length.
 *
 * output:
 *
 * return: 0 -- success, -1 -- failure
 *
 * ***********************************************/
int kerSysOldImageCfgSet(char *string, int strLen)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write old image config file to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_OLD_IMAGE_FILE_NAME, string, strLen);
    }

    if (fInfo.flash_old_image_cfg_number_blk <= 0)
    {
        printk("No old image config file blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    /*
     * The old image config file does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the old image config file spans.
     */
    for (i=fInfo.flash_old_image_cfg_start_blk;
         i < (fInfo.flash_old_image_cfg_start_blk + fInfo.flash_old_image_cfg_number_blk); i++)
    {
       usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

#ifdef GPL_CODE
    if (strLen > usedBlkSize)
    {
        printk("kerSysOldImageCfgSet strLen = %d, usedBlkSize = %d\n", strLen, usedBlkSize);
        return -1;
    }
#endif
    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy(pBuf , string, strLen);

    if (setSharedBlks(fInfo.flash_old_image_cfg_start_blk, fInfo.flash_old_image_cfg_number_blk, 
                      pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}
#endif

int kerSysBackupPsiSet(char *string, int strLen, int offset)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write backup PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_BACKUP_FILE_NAME, string, strLen);
    }

    if (fInfo.flash_backup_psi_number_blk <= 0)
    {
        printk("No backup psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    /*
     * The backup PSI does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the backup PSI spans.
     */
    for (i=fInfo.flash_backup_psi_start_blk;
         i < (fInfo.flash_backup_psi_start_blk + fInfo.flash_backup_psi_number_blk); i++)
    {
       usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

#ifdef GPL_CODE
    if (strLen + offset > usedBlkSize)
    {
        printk("kerSysBackupPsiSet strLen = %d, offset = %d, usedBlkSize = %d\n", strLen, offset, usedBlkSize);
        return -1;
    }
#endif
    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_backup_psi_start_blk, fInfo.flash_backup_psi_number_blk, 
                      pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}


/*******************************************************************************
 * "Kernel Syslog" is one or more sectors allocated in the flash
 * so that we can persist crash dump or other system diagnostics info
 * across reboots.  This feature is current not implemented.
 *******************************************************************************/

#if defined(GPL_CODE)
#define SYSLOG_FILE_NAME        "/data/syslog"
#else
#define SYSLOG_FILE_NAME        "/etc/syslog"
#endif

int kerSysSyslogGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read syslog from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;

        memset(string, 0x00, strLen);
        fp = filp_open(SYSLOG_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos) <= 0)
#if defined(GPL_CODE)
                printk("Failed to read syslog from '%s'\n", SYSLOG_FILE_NAME);
#else
                printk("Failed to read psi from '%s'\n", SYSLOG_FILE_NAME);
#endif
            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if (fInfo.flash_syslog_number_blk <= 0)
    {
        printk("No syslog blks allocated, change it in CFE\n");
        return -1;
    }
    
    if (strLen > fInfo.flash_syslog_length)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_syslog_start_blk,
                              fInfo.flash_syslog_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

int kerSysSyslogSet(char *string, int strLen, int offset)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write PSI to
         * a file on the NAND flash.
         */

#if defined(GPL_CODE)
        return kerSysFsFileSet(SYSLOG_FILE_NAME, string, strLen);
#else
        return kerSysFsFileSet(PSI_FILE_NAME, string, strLen);
#endif

    }

    if (fInfo.flash_syslog_number_blk <= 0)
    {
        printk("No syslog blks allocated, change it in CFE\n");
        return -1;
    }
    
    if (strLen > fInfo.flash_syslog_length)
        return -1;

    /*
     * The syslog does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the syslog spans.
     */
    for (i=fInfo.flash_syslog_start_blk;
         i < (fInfo.flash_syslog_start_blk + fInfo.flash_syslog_number_blk); i++)
    {
        usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_syslog_start_blk, fInfo.flash_syslog_number_blk, pBuf) != 0)
        sts = -1;

    retriedKfree(pBuf);

    return sts;
}

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268)
int otp_is_btrm_boot(void)
{
    int btrm_boot = (*((uint32_t *)(OTP_BASE + OTP_SHADOW_ADDR_BTRM_ENABLE_CUST_ROW)));
    btrm_boot = (btrm_boot & OTP_CUST_BTRM_BOOT_ENABLE_MASK) >> OTP_CUST_BTRM_BOOT_ENABLE_SHIFT;
    return btrm_boot;
}

int otp_is_boot_secure(void)
{
    int boot_secure = otp_is_btrm_boot();
    if (boot_secure)
    {
        /* Bootrom is enabled ... check mid */
        boot_secure = (*((uint32_t *)(OTP_BASE + OTP_SHADOW_ADDR_MARKET_ID_CUST_ROW)));
        boot_secure = (boot_secure & OTP_MFG_MRKTID_OTP_BITS_MASK) >> OTP_MFG_MRKTID_OTP_BITS_SHIFT;
    }
    return boot_secure;
}

#elif defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)

#define OTP_CUST_BTRM_BOOT_ENABLE_ROW           18
#define OTP_CUST_BTRM_BOOT_ENABLE_SHIFT         15
#define OTP_CUST_BTRM_BOOT_ENABLE_MASK          (0x7 << OTP_CUST_BTRM_BOOT_ENABLE_SHIFT)

#define OTP_CUST_MFG_MRKTID_ROW                 24
#define OTP_CUST_MFG_MRKTID_SHIFT               0
#define OTP_CUST_MFG_MRKTID_MASK                (0xffff << OTP_CUST_MFG_MRKTID_SHIFT)

int otp_retrieve(int row, int mask);
int otp_retrieve(int row, int mask)
{
   int	    rval = 0;
   uint32_t cntr = BTRM_OTP_READ_TIMEOUT_CNT;

   /* turn on cpu mode, set up row addr, activate read word */
   JTAG_OTP->ctrl1 |= JTAG_OTP_CTRL_CPU_MODE;
   JTAG_OTP->ctrl3 = row;
   JTAG_OTP->ctrl0 = JTAG_OTP_CTRL_START | JTAG_OTP_CTRL_PROG_EN | JTAG_OTP_CTRL_ACCESS_MODE;

   /* Wait for low CMD_DONE (current operation has begun), reset countdown, wait for retrieval to complete */
   while((JTAG_OTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE)
   {
      cntr--;
      if (cntr == 0)
          break;
   }
   cntr = BTRM_OTP_READ_TIMEOUT_CNT;
   while(!((JTAG_OTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE))
   {
      cntr--;
      if (cntr == 0)
         break;
   }

   /* If cntr nonzero, read was successful, retrieve data */
   if (cntr)
      rval = JTAG_OTP->status0;

   /* zero out the ctrl_0 reg, turn off cpu mode, return results */
   JTAG_OTP->ctrl0 = 0;
   JTAG_OTP->ctrl1 &= ~JTAG_OTP_CTRL_CPU_MODE;
   if (rval & mask)
      return 1;
   else
      return 0;
}

int otp_is_btrm_boot(void)
{
   int rval = otp_retrieve( OTP_CUST_BTRM_BOOT_ENABLE_ROW, OTP_CUST_BTRM_BOOT_ENABLE_MASK);
   return rval;
}

int otp_is_boot_secure(void)
{
    int rval = otp_is_btrm_boot();
    if (rval)
    {
        /* Bootrom is enabled ... check mid */
        rval = otp_retrieve( OTP_CUST_MFG_MRKTID_ROW, OTP_CUST_MFG_MRKTID_MASK);
    }
    return rval;
}

#if defined(CONFIG_BCM96848)
int otp_get_row(int row)
{
   uint32_t rval = 0;
   uint32_t cntr = BTRM_OTP_READ_TIMEOUT_CNT;

   /* turn on cpu mode, set up row addr, activate read word */
   JTAG_OTP->ctrl1 |= JTAG_OTP_CTRL_CPU_MODE;
   JTAG_OTP->ctrl3 = row;
   JTAG_OTP->ctrl0 = JTAG_OTP_CTRL_START | JTAG_OTP_CTRL_PROG_EN | JTAG_OTP_CTRL_ACCESS_MODE;

   /* Wait for low CMD_DONE (current operation has begun), reset countdown, wait for retrieval to complete */
   while((JTAG_OTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE)
   {
      cntr--;
      if (cntr == 0)
          break;
   }
   cntr = BTRM_OTP_READ_TIMEOUT_CNT;
   while(!((JTAG_OTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE))
   {
      cntr--;
      if (cntr == 0)
         break;
   }

   /* If cntr nonzero, read was successful, retrieve data */
   if (cntr)
      rval = JTAG_OTP->status0;

   /* zero out the ctrl_0 reg, turn off cpu mode, return results */
   JTAG_OTP->ctrl0 = 0;
   JTAG_OTP->ctrl1 &= ~JTAG_OTP_CTRL_CPU_MODE;

   return rval;
}
#endif

#endif


/*******************************************************************************
 * Writing software image to flash operations
 * This procedure should be serialized.  Look for flashImageMutex.
 *******************************************************************************/


#define je16_to_cpu(x) ((x).v16)
#define je32_to_cpu(x) ((x).v32)

#if defined(GPL_CODE_CONFIG_JFFS)
/*******************************************************************************
 * get seq(cferam.xxx) by partition number for JFFS
 *******************************************************************************/
int get_seq_frompartiion(int imageNumber)
{
    char *buf = NULL;
    int ret = -1;
    struct mtd_info *mtd=NULL;

    if ( imageNumber == 1 )
        mtd=get_mtd_device_nm("tag");
    else
        mtd=get_mtd_device_nm("tag_update");

    if (NULL == (buf = kmalloc(mtd->erasesize, GFP_KERNEL)))
    {
        printk("failed to allocate memory with size: %d\n", mtd->erasesize);
        return ret;
    }
    
    if (mtd && buf)
    {
        struct jffs2_raw_dirent *pdir; 
        char fname[] = NAND_CFE_RAM_NAME;
        int fname_actual_len = strlen(fname);
        int fname_cmp_len = strlen(fname) - 3; /* last three are digits */
        unsigned long version = 0;
        int blk_addr = 0;

        /*cferam.xxx blocks is follow right after the tag blocks, so need +1*/
        int total_search_blks = (AEI_TAG_BLOCKS+1) * mtd->erasesize;
        int retlen;


        /*there is at least one tag blocks, search froam blocks 2*/
        for( blk_addr = mtd->erasesize; blk_addr < total_search_blks; blk_addr += mtd->erasesize )
        {
           memset(buf, 0, mtd->erasesize);
           mtd_read(mtd, blk_addr, mtd->erasesize, &retlen, buf);

           pdir = (struct jffs2_raw_dirent *) buf;
           if( je16_to_cpu(pdir->magic) == JFFS2_MAGIC_BITMASK )
           {
              if( je16_to_cpu(pdir->nodetype) ==
                  JFFS2_NODETYPE_DIRENT &&
                   fname_actual_len == pdir->nsize &&
                   !memcmp(fname, pdir->name, fname_cmp_len) &&
                   je32_to_cpu(pdir->version) > version &&
                   je32_to_cpu(pdir->ino) != 0 )
                {
                    unsigned char *seq = pdir->name + fname_cmp_len;
                    ret = simple_strtoul(seq, NULL, 10);
                    version = je32_to_cpu(pdir->version);
                }
            }
         }
   
        put_mtd_device(mtd);
    }

    if (buf)
       kfree(buf);
    
    return ret;
}
#endif
/*
 * nandUpdateSeqNum
 * 
 * Read the sequence number from each rootfs partition.  The sequence number is
 * the extension on the cferam file.  Add one to the highest sequence number
 * and change the extenstion of the cferam in the image to be flashed to that
 * number.
 */
static char *nandUpdateSeqNum(unsigned char *imagePtr, int imageSize, int blkLen)
{
    char fname[] = NAND_CFE_RAM_NAME;
    int fname_actual_len = strlen(fname);
    int fname_cmp_len = strlen(fname) - 3; /* last three are digits */
    int seq = -1;
    int seq2 = -1;
    char *ret = NULL;

#if defined(GPL_CODE)
    int seqnum1=-1;
    int seqnum2=-1;
#endif
#if defined(GPL_CODE_CONFIG_JFFS) && defined(GPL_CODE_DUAL_IMAGE)
	PFILE_TAG pTag=(PFILE_TAG)(imagePtr);
#endif

#if defined(GPL_CODE_CONFIG_JFFS) && defined(GPL_CODE_DUAL_IMAGE)
    seq = simple_strtoul(pTag->imageSequence, NULL, 10);
#else
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
    /* If full secure boot is in play, the CFE RAM file is the encrypted version */
    int boot_secure = otp_is_boot_secure();
    if (boot_secure)
        strcpy(fname, NAND_CFE_RAM_SECBT_NAME);
#endif

    seq = kerSysGetSequenceNumber(1);
    seq2 = kerSysGetSequenceNumber(2);

#if defined(GPL_CODE_CONFIG_JFFS)
    /*maybe it can be think as a workaround, during 1 tag partion upgrading to 8 tag partition, cferam.xxx will be lost, it return -1.*/
    if ( seq == -1 && seq2 == -1 )
    {
       if ( (seq = get_seq_frompartiion(1)) == -1 ) 
       {
          printk("failed to get tag partition sequencenumber\n"); 
       }
       if ( (seq2 = get_seq_frompartiion(2)) == -1 )
       {
          printk("failed to get tag_update partition sequencenumber\n"); 
       }
    }
#endif

    /* deal with wrap around case */
    if ((seq == 0 && seq2 == 999) || (seq == 999 && seq2 == 0))
        seq = 0;
    else
        seq = (seq >= seq2) ? seq : seq2;

    if( seq != -1 )
#endif
    {
        unsigned char *buf, *p;
        struct jffs2_raw_dirent *pdir;
        unsigned long version = 0;
        int done = 0;

        while (( *(unsigned short *) imagePtr != JFFS2_MAGIC_BITMASK ) && (imageSize > 0))
        {
            imagePtr += blkLen;
            imageSize -= blkLen;
        }

#if defined(GPL_CODE_CONFIG_JFFS)
        /*skip image aei tag block*/
        if( *(unsigned short *) (imagePtr + 2) == AEI_MAGIC_BITMASK )
        {
            imagePtr += blkLen;
            imageSize -= blkLen;
        }
#endif

#if defined(GPL_CODE_CONFIG_JFFS) && defined(GPL_CODE_DUAL_IMAGE)
#else

	/* Confirm that we did find a JFFS2_MAGIC_BITMASK. If not, we are done */
	if (imageSize <= 0)
           done = 1;

        /* Increment the new highest sequence number. Add it to the CFE RAM
         * file name.
         */
        seq++;
        if (seq == 1000)
            seq = 0;

#if defined(GPL_CODE)
	//If the  seqnum is more than 999, the sequence number need be set to the small squence +1. 
        if(seq<=0)
          seq=0;

	//If the  seqnum is more than 999, the sequence number need be set to the small squence +1. 
        if(seq>=999)
        {
            if(seqnum1>seqnum2 && seqnum1!=(seqnum2+1))
                seq=seqnum2+1;
            else if (seqnum2>seqnum1 && seqnum2!=(seqnum1+1))
                seq=seqnum1+1;
	    else
                seq=0;

        }
#endif


#endif


        /* Search the image and replace the last three characters of file
         * cferam.000 with the new sequence number.
         */
        for(buf = imagePtr; buf < imagePtr+imageSize && done == 0; buf += blkLen)
        {
            p = buf;
            while( p < buf + blkLen )
            {
                pdir = (struct jffs2_raw_dirent *) p;
                if( je16_to_cpu(pdir->magic) == JFFS2_MAGIC_BITMASK )
                {
                    if( je16_to_cpu(pdir->nodetype) == JFFS2_NODETYPE_DIRENT &&
                        fname_actual_len == pdir->nsize &&
                        !memcmp(fname, pdir->name, fname_cmp_len) &&
                        je32_to_cpu(pdir->version) > version &&
                        je32_to_cpu(pdir->ino) != 0 )
                     {
                        /* File cferam.000 found. Change the extension to the
                         * new sequence number and recalculate file name CRC.
                         */
                        p = pdir->name + fname_cmp_len;
                        p[0] = (seq / 100) + '0';
                        p[1] = ((seq % 100) / 10) + '0';
                        p[2] = ((seq % 100) % 10) + '0';
                        p[3] = '\0';

                        je32_to_cpu(pdir->name_crc) =
                            crc32(0, pdir->name, (unsigned int)
                            fname_actual_len);

                        version = je32_to_cpu(pdir->version);

                        /* Setting 'done = 1' assumes there is only one version
                         * of the directory entry.
                         */
                        done = 1;
                        ret = buf;  /* Pointer to the block containing CFERAM directory entry in the image to be flashed */
                        break;
                    }

                    p += (je32_to_cpu(pdir->totlen) + 0x03) & ~0x03;
                }
                else
                {
                    done = 1;
                    break;
                }
            }
        }
    }
#if defined(GPL_CODE_CONFIG_JFFS)
    else
       printk("failed to get seq number from the partition, flash the highest sequence number failed!");
#endif

    return(ret);
}

/* Erase the specified NAND flash block. */
static int nandEraseBlk( struct mtd_info *mtd, int blk_addr )
{
    struct erase_info erase;
    int sts;

    /* Erase the flash block. */
    memset(&erase, 0x00, sizeof(erase));
    erase.addr = blk_addr;
    erase.len = mtd->erasesize;
    erase.mtd = mtd;

    if( (sts = mtd_block_isbad(mtd, blk_addr)) == 0 )
    {
        sts = mtd_erase(mtd, &erase);

        /* Function local_bh_disable has been called and this
         * is the only operation that should be occurring.
         * Therefore, spin waiting for erase to complete.
         */
        if( 0 == sts )
        {
            int i;

            for(i = 0; i < 10000 && erase.state != MTD_ERASE_DONE &&
                erase.state != MTD_ERASE_FAILED; i++ )
            {
                udelay(100);
            }

            if( erase.state != MTD_ERASE_DONE )
            {
                sts = -1;
                printk("nandEraseBlk - Block 0x%8.8x. Error erase block timeout.\n", blk_addr);
            }
        }
        else
            printk("nandEraseBlk - Block 0x%8.8x. Error erasing block.\n", blk_addr);
    }
    else
        printk("nandEraseBlk - Block 0x%8.8x. Bad block.\n", blk_addr);

    return (sts);
}

/* Write data with or without JFFS2 clean marker, must pass function an aligned block address */
static int nandWriteBlk(struct mtd_info *mtd, int blk_addr, int data_len, char *data_ptr, bool write_JFFS2_clean_marker)
{
#ifdef CONFIG_CPU_LITTLE_ENDIAN
    const unsigned short jffs2_clean_marker[] = {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0008, 0x0000};
#else
    const unsigned short jffs2_clean_marker[] = {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0000, 0x0008};
#endif
    struct mtd_oob_ops ops;
    int sts = 0;
    int page_addr, byte;

    for (page_addr = 0; page_addr < data_len; page_addr += mtd->writesize)
    {
        memset(&ops, 0x00, sizeof(ops));

        // check to see if whole page is FFs
        for (byte = 0; (byte < mtd->writesize) && ((page_addr + byte) < data_len); byte += 4)
        {
            if ( *(uint32 *)(data_ptr + page_addr + byte) != 0xFFFFFFFF )
            {
                ops.len = mtd->writesize < (data_len - page_addr) ? mtd->writesize : data_len - page_addr;
                ops.datbuf = data_ptr + page_addr;
                break;
            }
        }

        if (write_JFFS2_clean_marker)
        {
            ops.mode = MTD_OPS_AUTO_OOB;
            ops.oobbuf = (char *)jffs2_clean_marker;
            ops.ooblen = sizeof(jffs2_clean_marker);
            write_JFFS2_clean_marker = 0; // write clean marker to first page only
        }

        if (ops.len || ops.ooblen)
        {
            if( (sts = mtd_write_oob(mtd, blk_addr + page_addr, &ops)) != 0 )
            {
                printk("nandWriteBlk - Block 0x%8.8x. Error writing page.\n", blk_addr + page_addr);
                nandEraseBlk(mtd, blk_addr);
                mtd_block_markbad(mtd, blk_addr);
                break;
            }
        }
    }

    return(sts);
}

#if defined(GPL_CODE)
static void reportUpgradePercent(int percent)
{
        struct file     *f;

        long int        rv;
        mm_segment_t    old_fs;
        loff_t          offset=0;
        char buf[128] = "";

        sprintf(buf, "%d\n", percent);
        f = filp_open("/var/UpgradePercent", O_RDWR|O_CREAT|O_TRUNC, 0600);
        if(f == NULL)
                return;

        old_fs = get_fs();
        set_fs( get_ds() );
        rv = f->f_op->write(f, buf, strlen(buf), &offset);
        set_fs(old_fs);

        if (rv < strlen(buf)) {
                printk("write /var/UpgradePercent  error\n");
        }
        filp_close(f , 0);
        return;
}
#endif
// NAND flash bcm image 
// return: 
// 0 - ok
// !0 - the sector number fail to be flashed (should not be 0)
int kerSysBcmNandImageSet( char *rootfs_part, char *image_ptr, int img_size,
    int should_yield )
{
    int sts = -1;
    int blk_addr;
    int old_img = 0;
    char *cferam_ptr;
    int rsrvd_for_cferam;
    char *end_ptr = image_ptr + img_size;
    struct mtd_info *mtd0 = get_mtd_device_nm("image");
    WFI_TAG wt = {0};
    int nvramXferSize;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
    uint32_t  btrmEnabled = otp_is_btrm_boot();
#endif

#ifdef GPL_CODE_CONFIG_JFFS

    struct mtd_info *mtd_tag = NULL;
    char * block_buf = NULL;
    int retlen = 0;

#if defined(GPL_CODE_BACKUP_TAG)
#define BACKUP_TAG_OFFSET 512   //backup tag is sotred in the same block with tag of active firmware.
    struct mtd_info *mtd_backup = NULL;
    char * tagbackup_buf = NULL;
#endif

#endif

    /* Reserve room to flash block containing directory entry for CFERAM. */
    rsrvd_for_cferam = 8 * mtd0->erasesize;

    if( !IS_ERR_OR_NULL(mtd0) )
    {
        unsigned int chip_id = kerSysGetChipId();
        int blksize = mtd0->erasesize / 1024;

        memcpy(&wt, end_ptr, sizeof(wt));

#if defined(CHIP_FAMILY_ID_HEX)
        chip_id = CHIP_FAMILY_ID_HEX;
#endif

        if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            wt.wfiChipId != chip_id )
        {
            int id_ok = 0;

            if (id_ok == 0) {
                printk("Chip Id error.  Image Chip Id = %04x, Board Chip Id = "
                    "%04x.\n", wt.wfiChipId, chip_id);
                put_mtd_device(mtd0);
                return -1;
            }
        }
        else if( wt.wfiFlashType == WFI_NOR_FLASH )
        {
            printk("\nERROR: Image does not support a NAND flash device.\n\n");
            put_mtd_device(mtd0);
            return -1;
        }
        else if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            ((wt.wfiFlashType < WFI_NANDTYPE_FLASH_MIN && wt.wfiFlashType > WFI_NANDTYPE_FLASH_MAX) ||
              blksize != WFI_NANDTYPE_TO_BKSIZE(wt.wfiFlashType) ) )
        {
            printk("\nERROR: NAND flash block size %dKB does not work with an "
                "image built with %dKB block size\n\n", blksize,WFI_NANDTYPE_TO_BKSIZE(wt.wfiFlashType));
            put_mtd_device(mtd0);
            return -1;
        }
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
	else if (((  (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM)) && (! btrmEnabled)) ||
                 ((! (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM)) && (  btrmEnabled)))
        {
            printk("ERROR: Secure Boot Conflict. The image type does not match the SoC OTP configuration. Aborting.\n");
            put_mtd_device(mtd0);
            return -1;
        }
#endif
        else
        {
            mtd0 = get_mtd_device_nm(rootfs_part);

            /* If the image version indicates that it uses a 1MB data partition
             * size and the image is intended to be flashed to the second file
             * system partition, change to the flash to the first partition.
             * After new image is flashed, delete the second file system and
             * data partitions (at the bottom of this function).
             */
            if( wt.wfiVersion == WFI_VERSION_NAND_1MB_DATA )
            {
                unsigned long rootfs_ofs;
                kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs);
                
                if(rootfs_ofs == inMemNvramData.ulNandPartOfsKb[NP_ROOTFS_1] &&
                    mtd0)
                {
                    printk("Old image, flashing image to first partition.\n");
                    put_mtd_device(mtd0);
                    mtd0 = NULL;
                    old_img = 1;
                }
            }

            if( IS_ERR_OR_NULL(mtd0) || mtd0->size == 0LL )
            {
                /* Flash device is configured to use only one file system. */
                if( !IS_ERR_OR_NULL(mtd0) )
                    put_mtd_device(mtd0);
                mtd0 = get_mtd_device_nm("image");
            }
        }
    }

#ifdef DEBUG_FLASH
    printk("----> rootfs_part = %s, mtd0->name = %s\n", rootfs_part, mtd0->name);
#endif

#ifdef GPL_CODE_CONFIG_JFFS
    if( !IS_ERR_OR_NULL(mtd0) )
    {
        if ((rootfs_part != NULL) && ((strcmp(rootfs_part, "image") == 0) || (strcmp(rootfs_part, "rootfs") == 0)) )
        {
            mtd_tag = get_mtd_device_nm("tag");
#if defined(GPL_CODE_BACKUP_TAG)
            mtd_backup=get_mtd_device_nm("tag_update");
#endif
        }
        else
        {
            mtd_tag = get_mtd_device_nm("tag_update");
#if defined(GPL_CODE_BACKUP_TAG)
            mtd_backup = get_mtd_device_nm("tag");
#endif
        }
    }
#endif

    if( !IS_ERR_OR_NULL(mtd0) )
    {
        int ofs;
        unsigned long flags=0;
        int writelen;
        int writing_ubifs;

        if( *(unsigned short *) image_ptr == JFFS2_MAGIC_BITMASK )
        { /* Downloaded image does not contain CFE ROM boot loader */
            ofs = 0;
        }
        else
        {
            /* Downloaded image contains CFE ROM boot loader. */
            PNVRAM_DATA pnd = (PNVRAM_DATA) (image_ptr + NVRAM_SECTOR*((unsigned int)flash_get_sector_size(0))+NVRAM_DATA_OFFSET);

            ofs = mtd0->erasesize;
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
            /* check if it is zero padded for backward compatiblity */
           if( (wt.wfiFlags&WFI_FLAG_HAS_PMC) == 0 )
           {
               unsigned int *pImg  = (unsigned int*)image_ptr;
               unsigned char * pBuf = image_ptr;
               int block_start, block_end, remain, block;
               struct mtd_info *mtd1;
               int readlen;
              
               if( *pImg == 0 && *(pImg+1) == 0 && *(pImg+2) == 0 && *(pImg+3) == 0 )
               {
                   /* the first 64KB are for PMC in 631x8, need to preserve that for cfe/linux image update if it is not for PMC image update. */
                   block_start = 0;
                   block_end = IMAGE_OFFSET/mtd0->erasesize;
                   remain = IMAGE_OFFSET%mtd0->erasesize;

                   mtd1 = get_mtd_device_nm("nvram");
                   if( !IS_ERR_OR_NULL(mtd1) )
                   {
                       for( block = block_start; block < block_end; block++ )
                       {
                           mtd_read(mtd1, block*mtd0->erasesize, mtd0->erasesize, &readlen, pBuf);
                           pBuf += mtd0->erasesize;
                       }

                       if( remain )
                       {
                           block = block_end;
                           mtd_read(mtd1, block*mtd0->erasesize, remain, &readlen, pBuf);
                       }

                       put_mtd_device(mtd1);
                   }
                   else 
                   {
                       printk("Failed to get nvram mtd device\n");
                       put_mtd_device(mtd0);
                       return -1;
                   }
               }
               else
               {
                   printk("Invalid NAND image.No PMC image or padding\n");
                   put_mtd_device(mtd0);
                   return -1;
               }
           }
#endif

            nvramXferSize = sizeof(NVRAM_DATA);
#if defined(CONFIG_BCM963268)
            if ((wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM) && otp_is_boot_secure())
            {
               /* Upgrading a secure-boot 63268 SoC. Nvram is 3k. do not preserve the old */
               /* security credentials kept in nvram but rather use the new credentials   */
               /* embedded within the new image (ie the last 2k of the 3k nvram) */
               nvramXferSize = 1024;
            }
#endif

            /* Copy NVRAM data to block to be flashed so it is preserved. */
            spin_lock_irqsave(&inMemNvramData_spinlock, flags);
            memcpy((unsigned char *) pnd, inMemNvramData_buf, nvramXferSize);
            spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

            /* Recalculate the nvramData CRC. */
            pnd->ulCheckSum = 0;
            pnd->ulCheckSum = crc32(CRC32_INIT_VALUE, pnd, sizeof(NVRAM_DATA));
        }

        /* 
         * Scan downloaded image for cferam.000 directory entry and change file externsion 
         * to cfe.YYY where YYY is the current cfe.XXX + 1. If full secure boot is in play,
	 * the file to be updated is secram.000 and not cferam.000
         */
        cferam_ptr = nandUpdateSeqNum((unsigned char *) image_ptr, img_size, mtd0->erasesize);

        if( cferam_ptr == NULL )
        {
            printk("\nERROR: Invalid image. ram.000 not found.\n\n");
            put_mtd_device(mtd0);
            return -1; 
        }

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
        if ((wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM) && (ofs != 0))
        {
            /* These targets support bootrom boots which is currently enabled. the "nvram" */
            /* mtd device may be bigger than just the first nand block. Check that the new */
            /* image plays nicely with the current partition table settings. */
            struct mtd_info *mtd1 = get_mtd_device_nm("nvram");
            if( !IS_ERR_OR_NULL(mtd1) )
            {
                uint32_t *pHdr = (uint32_t *)image_ptr;
                pHdr += (mtd1->erasesize / 4); /* pHdr points to the top of the 2nd nand block */
                for( blk_addr = mtd1->erasesize; blk_addr < mtd1->size; blk_addr += mtd1->erasesize )
                { 
                    /* If we are inside the for() loop, "nvram" mtd is larger than 1 block */
                    pHdr += (mtd1->erasesize / 4);
                }
   
                if (*(unsigned short *)pHdr != JFFS2_MAGIC_BITMASK)
                {
                    printk("New sw image does not match the partition table. Aborting.\n");
                    put_mtd_device(mtd0);
                    put_mtd_device(mtd1);
                    return -1; 
                }
                put_mtd_device(mtd1);
            }
	    else
            {
                printk("Failed to get nvram mtd device\n");
                put_mtd_device(mtd0);
                return -1;
            }
        }
#endif

        /*
         * All checks complete ... write image to flash memory.
         * In theory, all calls to flash_write_buf() must be done with
         * semflash held, so I added it here.  However, in reality, all
         * flash image writes are protected by flashImageMutex at a higher
         * level.
         */
        down(&semflash);

        // Once we have acquired the flash semaphore, we can
        // disable activity on other processor and also on local processor.
        // Need to disable interrupts so that RCU stall checker will not complain.
        if (!should_yield)
        {
#if !defined(GPL_CODE)
            stopOtherCpu();
#endif
            local_irq_save(flags);
        }

#if defined(GPL_CODE)
	// do nothing here
#else
        local_bh_disable();
#endif

        if( 0 != ofs ) /* Image contains CFE ROM boot loader. */
        {
            /* Prepare to flash the CFE ROM boot loader. */
            struct mtd_info *mtd1 = get_mtd_device_nm("nvram");
            if( !IS_ERR_OR_NULL(mtd1) )
            {
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
                StallPmc();
#endif
                if (nandEraseBlk(mtd1, 0) == 0)
                    nandWriteBlk(mtd1, 0,  mtd1->erasesize, image_ptr,TRUE);

                image_ptr += ofs;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
                if (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM)
                {
                   /* We have already checked that the new sw image matches the partition table. Therefore */
		   /* burn the rest of the "nvram" mtd (if any) */
                   for( blk_addr = mtd1->erasesize; blk_addr < mtd1->size; blk_addr += mtd1->erasesize )
                   { 
                      if (nandEraseBlk( mtd1, blk_addr ) == 0)
		      { 
                         nandWriteBlk(mtd1, blk_addr, mtd1->erasesize, image_ptr, TRUE);
                         image_ptr += ofs;
                      }
                   }
                }
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
                UnstallPmc();
#endif

                put_mtd_device(mtd1);
            }
            else
            {
                printk("Failed to get nvram mtd device\n");
                put_mtd_device(mtd0);
                return -1;
            }
        }

#ifdef GPL_CODE_CONFIG_JFFS
        if((*(unsigned short *) image_ptr == JFFS2_MAGIC_BITMASK) && (*(unsigned short *) (image_ptr + 2) == AEI_MAGIC_BITMASK) && mtd_tag != NULL)
        {
            int total_tag_blks = AEI_TAG_BLOCKS * mtd_tag->erasesize;

#if defined(GPL_CODE_BACKUP_TAG)
            /* before we erase tag partition of inactive firmware,we have to read backup tag of active firmware out ,
             * and write it back to flas with  new tag together */
            if (NULL != (tagbackup_buf = kmalloc(mtd_tag->erasesize, GFP_KERNEL)) )
            {
                memset(tagbackup_buf, 0, mtd_tag->erasesize);
                /* read tag partition of inactive firmware to the buffer */
                mtd_read(mtd_tag, 0, mtd_tag->erasesize, &retlen, tagbackup_buf);
                /* copy backup tag of active firmware to the buffer that will wirte to flash */
                memcpy((unsigned char *) tagbackup_buf , image_ptr, sizeof(FILE_TAG));
            }
#endif

//            nandEraseBlk(mtd_tag, 0);
            /* Erase tag blocks before flashing the image. */
            for( blk_addr = 0; blk_addr < total_tag_blks; blk_addr += mtd_tag->erasesize )
               nandEraseBlk( mtd_tag, blk_addr );

            printk("##Write tag start,mtd->name(%s)\n", mtd_tag->name);
            /* Flash the image header. */
#if defined(GPL_CODE_BACKUP_TAG)
            if(tagbackup_buf != NULL)
                nandWriteBlk(mtd_tag, 0, mtd_tag->erasesize, tagbackup_buf, TRUE);
            else
                nandWriteBlk(mtd_tag, 0, mtd_tag->erasesize, image_ptr, TRUE);
#else
            for( blk_addr = 0; blk_addr < total_tag_blks; blk_addr += mtd_tag->erasesize )
            {
                printk("##Write tag %8.8x\n", blk_addr);
                if (nandWriteBlk(mtd_tag, blk_addr, mtd_tag->erasesize, image_ptr, TRUE) != 0 )
                   printk("Error writing Block 0x%8.8x\n", blk_addr);
            }
#endif

            // Because we don't disable the interrupt, NAND flash write is unreliable. 
            // We add to check the correctness of NAND flash writing by the software.
            do
            {
                if (NULL != (block_buf = kmalloc(mtd_tag->erasesize, GFP_KERNEL)) )
                {
                    memset(block_buf, 0, mtd_tag->erasesize);
                    mtd_read(mtd_tag, 0, mtd_tag->erasesize, &retlen, block_buf);
                    // printk("#####block(%d),image(%d)\n",inMemNvramData.ulNandPartOfsKb[image_number]/inMemNvramData.ulNandPartSizeKb[NP_BOOT], \
                       image_number);
#if defined(GPL_CODE_BACKUP_TAG)
                    if((tagbackup_buf == NULL && memcmp(block_buf, image_ptr,  mtd_tag->erasesize) !=0 ) ||
                       (tagbackup_buf != NULL && memcmp(block_buf, tagbackup_buf, mtd_tag->erasesize) != 0) )
#else
                    if(memcmp(block_buf, image_ptr, mtd_tag->erasesize) !=0 )
#endif
                    {
                        printk("##write tag error\n");
                        nandEraseBlk(mtd_tag, 0 );

#if defined(GPL_CODE_BACKUP_TAG)
                        if(tagbackup_buf != NULL)
                            nandWriteBlk(mtd_tag, 0, mtd_tag->erasesize, tagbackup_buf, TRUE);
                        else
                            nandWriteBlk(mtd_tag, 0, mtd_tag->erasesize, image_ptr, TRUE);
#else
                            nandWriteBlk(mtd_tag, 0, mtd_tag->erasesize, image_ptr, TRUE);
#endif

                        kfree(block_buf);
                    }
                    else
                    {
                        printk("##write tag correct\n");
                        kfree(block_buf);
                        break;
                    }
                }
                else
                {
                     printk("##malloc fail\n");
                     break;
                }
            }
            while(1);

//store backup tag
#if defined(GPL_CODE_BACKUP_TAG)
            if(tagbackup_buf)
            {
                kfree(tagbackup_buf);
                tagbackup_buf = NULL;
            }

            if(mtd_backup)
            {
                if (NULL !=(tagbackup_buf = kmalloc(mtd_backup->erasesize, GFP_KERNEL)) )
                {
                     memset(tagbackup_buf, 0, mtd_backup->erasesize);
                     /* read tag of active partition to the buffer */
                     mtd_read(mtd_backup, 0, mtd_backup->erasesize, &retlen, tagbackup_buf);
                     /* copy current tag to the buffer,after tag 512 byte */
                     memcpy((unsigned char *) tagbackup_buf + BACKUP_TAG_OFFSET, image_ptr, sizeof(FILE_TAG));
                     nandEraseBlk(mtd_backup, 0 );
                     printk("start to write backup tag\n");
                     /* write backup tag to flash */
                     nandWriteBlk(mtd_backup, 0, mtd_backup->erasesize, tagbackup_buf, TRUE);

                     /* Because we don't disable the interrupt, NAND flash write is unreliable. 
                      * We add to check the correctness of NAND                            flash writing by the software. */
                     do
                     {
                         if (NULL != (block_buf = kmalloc(mtd_backup->erasesize, GFP_KERNEL)) )
                         {
                              memset(block_buf, 0, mtd_backup->erasesize);
                              mtd_read(mtd_backup, 0, mtd_backup->erasesize, &retlen, block_buf);

                              if(memcmp(block_buf, tagbackup_buf, mtd_backup->erasesize) != 0 )
                              {
                                  printk("##write backup tag error\n");
                                  nandEraseBlk(mtd_backup, 0);
                                  nandWriteBlk(mtd_backup, 0, mtd_backup->erasesize, tagbackup_buf, TRUE);
                                  kfree(block_buf);
                              }
                              else
                              {
                                  printk("##write tag correct\n");
                                  kfree(block_buf);
                                  break;
                              }
                         }
                         else
                         {
                              printk("##malloc fail\n");
                              break;
                         }
                     }
                     while(1);

                     kfree(tagbackup_buf);
                }
            }
#endif
            image_ptr += mtd_tag->erasesize;
        }
#endif


        /* Erase blocks containing directory entry for CFERAM before flashing the image. */
        for( blk_addr = 0; blk_addr < rsrvd_for_cferam; blk_addr += mtd0->erasesize )
            nandEraseBlk( mtd0, blk_addr );

        /* Flash the image except for CFERAM directory entry, during which all the blocks in the partition (other than CFE) will be erased */
        writing_ubifs = 0;
        for( blk_addr = rsrvd_for_cferam; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        {
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
            { // block was erased successfully
                if ( image_ptr == cferam_ptr )
                { // skip CFERAM directory entry block
                    image_ptr += mtd0->erasesize;
                }
                else
                { /* Write a block of the image to flash. */
                    if( image_ptr < end_ptr )
                    { // if any data left, prepare to write it out
                        writelen = ((image_ptr + mtd0->erasesize) <= end_ptr)
                            ? mtd0->erasesize : (int) (end_ptr - image_ptr);
                    }
                    else
                        writelen = 0;

                    if (writelen) /* Write data with or without JFFS2 clean marker */
                    {
                        /* Write clean markers only to JFFS2 blocks */
                        if (nandWriteBlk(mtd0, blk_addr, writelen, image_ptr, !writing_ubifs) != 0 )
                            printk("Error writing Block 0x%8.8x\n", blk_addr);
                        else
                        { // successful write
                            printk(".");

                            if ( writelen )
                            { // increment counter and check for UBIFS split marker if data was written
                                image_ptr += writelen;
#if defined(GPL_CODE)
                        reportUpgradePercent(100-(unsigned int) (end_ptr - image_ptr)*100/img_size);
#endif

                                if (!strncmp(BCM_BCMFS_TAG, image_ptr - 0x100, strlen(BCM_BCMFS_TAG)))
                                    if (!strncmp(BCM_BCMFS_TYPE_UBIFS, image_ptr - 0x100 + strlen(BCM_BCMFS_TAG), strlen(BCM_BCMFS_TYPE_UBIFS)))
                                    { // check for UBIFS split marker
                                        writing_ubifs = 1;
                                        printk("U");
                                    }
                            }
                        }
                    }

                    if (should_yield)
                    {
                        local_bh_enable();
                        yield();
                        local_bh_disable();
                    }
                }
            }
        }

        /* Flash the block containing directory entry for CFERAM. */
        for( blk_addr = 0; blk_addr < rsrvd_for_cferam; blk_addr += mtd0->erasesize )
        {
            if (mtd_block_isbad(mtd0, blk_addr) == 0)
            {
                /* Write a block of the image to flash. */
                if (nandWriteBlk(mtd0, blk_addr, cferam_ptr ? mtd0->erasesize : 0, cferam_ptr, TRUE) == 0)
                {
                    printk(".");
                    cferam_ptr = NULL;
                }

                if (should_yield)
                {
                    local_bh_enable();
                    yield();
                    local_bh_disable();
                }
            }
        }

        if( cferam_ptr == NULL )
        {
            /* block containing directory entry for CFERAM was written successfully! */
            /* Whole flash image is programmed successfully! */
            sts = 0;
        }

        up(&semflash);
        printk("\n\n");

        if (should_yield)
        {
            local_bh_enable();
        }

        if( sts )
        {
            /*
             * Even though we try to recover here, this is really bad because
             * we have stopped the other CPU and we cannot restart it.  So we
             * really should try hard to make sure flash writes will never fail.
             */
            printk(KERN_ERR "nandWriteBlk: write failed at blk=%d\n", blk_addr);
            sts = (blk_addr > mtd0->erasesize) ? blk_addr / mtd0->erasesize : 1;
            if (!should_yield)
            {
                local_irq_restore(flags);
                local_bh_enable();
            }
        }
    }

    if( !IS_ERR_OR_NULL(mtd0) )
        put_mtd_device(mtd0);

#ifdef GPL_CODE_CONFIG_JFFS
    if( !IS_ERR_OR_NULL(mtd_tag) )
        put_mtd_device(mtd_tag);
#if defined(GPL_CODE_BACKUP_TAG)
    if( !IS_ERR_OR_NULL(mtd_backup) )
        put_mtd_device(mtd_backup);
#endif
#endif 


    if( sts == 0 && old_img == 1 )
    {
        printk("\nOld image, deleting data and secondary file system partitions\n");
        mtd0 = get_mtd_device_nm("data");
        for( blk_addr = 0; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        { // write JFFS2 clean markers
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
                nandWriteBlk(mtd0, blk_addr, 0, NULL, TRUE);
        }

        mtd0 = get_mtd_device_nm("image_update");
        for( blk_addr = 0; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        {
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
                nandWriteBlk(mtd0, blk_addr, 0, NULL, TRUE);
        }
    }

    return sts;
}

    // NAND flash overwrite nvram block.    
    // return: 
    // 0 - ok
    // !0 - the sector number fail to be flashed (should not be 0)
static int nandNvramSet(const char *nvramString )
{
    /* Image contains CFE ROM boot loader. */
    struct mtd_info *mtd = get_mtd_device_nm("nvram"); 
    char *cferom_ptr = NULL;
    int retlen = 0;
    
    if( IS_ERR_OR_NULL(mtd) )
        return -1;
    
    if ( (cferom_ptr = (char *)retriedKmalloc(mtd->erasesize)) == NULL )
    {
        printk("\n Failed to allocate memory in nandNvramSet();");
        return -1;
    }

    /* Read the whole cfe rom nand block 0 */
    mtd_read(mtd, 0, mtd->erasesize, &retlen, cferom_ptr);

    /* Copy the nvram string into place */
    memcpy(cferom_ptr + NVRAM_DATA_OFFSET, nvramString, sizeof(NVRAM_DATA));
    
    /* Flash the CFE ROM boot loader. */
    if (nandEraseBlk( mtd, 0 ) == 0)
        nandWriteBlk(mtd, 0, mtd->erasesize, cferom_ptr, TRUE);

    retriedKfree(cferom_ptr);
    return 0;
}
           

// flash bcm image 
// return: 
// 0 - ok
// !0 - the sector number fail to be flashed (should not be 0)
// Must be called with flashImageMutex held.
int kerSysBcmImageSet( int flash_start_addr, char *string, int size,
    int should_yield)
{
    int sts;
    int sect_size;
    int blk_start;
    int savedSize = size;
    int whole_image = 0;
    unsigned long flags=0;
    int is_cfe_write=0;
    WFI_TAG wt = {0};
#if defined(GPL_CODE)
    int whole_size = size;
#endif

#if defined(CONFIG_BCM963268)
    PNVRAM_DATA pnd;
#endif

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if (flash_start_addr == FLASH_BASE)
    {
        unsigned int chip_id = kerSysGetChipId();
        whole_image = 1;
        memcpy(&wt, string + size, sizeof(wt));
        if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            wt.wfiChipId != chip_id )
        {
            int id_ok = 0;
#if defined(CHIP_FAMILY_ID_HEX)
            if (wt.wfiChipId == CHIP_FAMILY_ID_HEX) {
                id_ok = 1;
            }
#endif
            if (id_ok == 0) {
                printk("Chip Id error.  Image Chip Id = %04x, Board Chip Id = "
                    "%04x.\n", wt.wfiChipId, chip_id);
                return -1;
            }
        }
    }

    if( whole_image && (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
        wt.wfiFlashType != WFI_NOR_FLASH )
    {
        printk("ERROR: Image does not support a NOR flash device.\n");
        return -1;
    }

#if defined(CONFIG_BCM963268)
    if ( whole_image && ( (   (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM)  && (! otp_is_btrm_boot())) ||
                          ((! (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM)) &&    otp_is_btrm_boot() ) ))
    {
        printk("ERROR: Secure Boot Conflict. The image type does not match the SoC OTP configuration. Aborting.\n");
        return -1;
    }
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* check if it is zero padded or no 64K padding at all for backward compatiblity */
    if(whole_image && ( (wt.wfiFlags&WFI_FLAG_HAS_PMC) == 0) )
    {
        unsigned int *pImg  = (unsigned int*)string;
        unsigned char * pBuf = string;
        int block_start, block_end, sect_size, remain, block;
        
	if( *pImg == 0 && *(pImg+1) == 0 && *(pImg+2) == 0 && *(pImg+3) == 0 )
	{
	   /* the first 64KB are for PMC in 631x8, need to preserve that for cfe/linux image update
           if it is not for PMC image update. */
           sect_size = flash_get_sector_size(0);
           block_start = 0;
           block_end = IMAGE_OFFSET/sect_size;
           remain = IMAGE_OFFSET%sect_size;

           for( block = block_start; block < block_end; block++ )
           {
               flash_read_buf(block, 0, pBuf, sect_size);
               pBuf += sect_size;
           }
		 
           if( remain )
           {
   	       block = block_end;
               flash_read_buf(block, 0, pBuf, remain);
           }
	}
	else
	{
            /* does not have PMC at all, the input data starting from 64KB offset */
	    if( (flash_get_flash_type() == FLASH_IFC_SPI) || (flash_get_flash_type() == FLASH_IFC_HS_SPI)  )
  	        flash_start_addr += IMAGE_OFFSET;
	}
    }
#endif

#if defined(DEBUG_FLASH)
    printk("kerSysBcmImageSet: flash_start_addr=0x%x string=%p len=%d whole_image=%d\n",
           flash_start_addr, string, size, whole_image);
#endif

    blk_start = flash_get_blk(flash_start_addr);

    if( blk_start < 0 )
        return( -1 );

    is_cfe_write = ((NVRAM_SECTOR == blk_start) &&
                    (size <= FLASH_LENGTH_BOOT_ROM));
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* if PMC image is included, it is also considered as CFE write */
    if( blk_start == 0 && size <= (FLASH_LENGTH_BOOT_ROM+IMAGE_OFFSET) )
	is_cfe_write = 1;
#endif

    /*
     * write image to flash memory.
     * In theory, all calls to flash_write_buf() must be done with
     * semflash held, so I added it here.  However, in reality, all
     * flash image writes are protected by flashImageMutex at a higher
     * level.
     */
    down(&semflash);


    // Once we have acquired the flash semaphore, we can
    // disable activity on other processor and also on local processor.
    // Need to disable interrupts so that RCU stall checker will not complain.
    if (!is_cfe_write && !should_yield)
    {
#ifdef GPL_CODE
    //If stop other CPU before beginning writing flash,
	//it'll cause client Browser has no upgrade's status!
#else
        stopOtherCpu();
#endif
        local_irq_save(flags);
    }

#if defined(GPL_CODE)
    // do nothing here
#else
    local_bh_disable();
#endif 

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* stall pmc if we are writing to the pmc flash address*/
    if( flash_start_addr == FLASH_BASE )
         StallPmc();
#endif
    do 
    {
        sect_size = flash_get_sector_size(blk_start);
	/* better to do read/modify/write for PMC code on 63138 at first 64K.
           so far SPI flash only has sector size up to 64KB. So we are ok for now */
        flash_sector_erase_int(blk_start);     // erase blk before flash

        if (sect_size > size) 
        {
            if (size & 1) 
                size++;
            sect_size = size;
        }

#if defined(CONFIG_BCM963268)
        if ((NVRAM_SECTOR == blk_start) && (wt.wfiFlags & WFI_FLAG_SUPPORTS_BTRM) && otp_is_boot_secure())
        {
            /* Upgrading a secure-boot 63268 SoC, and we are about to update the sector  */
            /* that contains the 3k nvram. Ensure the new security credentials make it   */
            /* into flash and the in-memory copy of nvram. Do this by making nvram valid */
            /* within the new image before writing it to flash */
            pnd = (PNVRAM_DATA)(string + NVRAM_DATA_OFFSET);
            spin_lock_irqsave(&inMemNvramData_spinlock, flags);
            memcpy((unsigned char *)pnd, inMemNvramData_buf, 1024); /* do not change 1024 to be sizeof(NVRAM_DATA) */
            spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
            pnd->ulCheckSum = 0;
            pnd->ulCheckSum = crc32(CRC32_INIT_VALUE, pnd, sizeof(NVRAM_DATA));
         }
#endif

        if (flash_write_buf(blk_start, 0, string, sect_size) != sect_size) {
            break;
        }

        // check if we just wrote into the sector where the NVRAM is.
        // update our in-memory copy
        if (NVRAM_SECTOR == blk_start)
        {
            updateInMemNvramData(string+NVRAM_DATA_OFFSET, NVRAM_LENGTH, 0);
        }
#if defined(GPL_CODE)
        reportUpgradePercent(100-size*100/whole_size);
#endif

        printk(".");
        blk_start++;
        string += sect_size;
        size -= sect_size; 

        if (should_yield)
        {
#if defined(GPL_CODE)
    // do nothing here
#else
            local_bh_enable();
#endif
            yield();
#if defined(GPL_CODE)
    // do nothing here
#else
            local_bh_disable();
#endif
        }
    } while (size > 0);

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* stall pmc if we are writing to the pmc flash address*/
    if( flash_start_addr == FLASH_BASE )
         UnstallPmc();
#endif
    if (whole_image && savedSize > fInfo.flash_rootfs_start_offset)
    {
        // If flashing a whole image, erase to end of flash.
        int total_blks = flash_get_numsectors();
        while( blk_start < total_blks )
        {
            flash_sector_erase_int(blk_start);
            printk(".");
            blk_start++;

            if (should_yield)
            {
#if defined(GPL_CODE)
    // do nothing here
#else
                local_bh_enable();
#endif
                yield();
#if defined(GPL_CODE)
    // do nothing here
#else
                local_bh_disable();
#endif
            }
        }
    }

    up(&semflash);

    printk("\n\n");

    if (is_cfe_write || should_yield)
    {
#if defined(GPL_CODE)
    // do nothing here
#else
        local_bh_enable();
#endif
    }

    if( size == 0 )
    {
        sts = 0;  // ok
    }
    else
    {
        /*
         * Even though we try to recover here, this is really bad because
         * we have stopped the other CPU and we cannot restart it.  So we
         * really should try hard to make sure flash writes will never fail.
         */
        printk(KERN_ERR "kerSysBcmImageSet: write failed at blk=%d\n",
                        blk_start);
        sts = blk_start;    // failed to flash this sector
        if (!is_cfe_write && !should_yield)
        {
            local_irq_restore(flags);
#if defined(GPL_CODE)
    // do nothing here
#else
            local_bh_enable();
#endif
        }
    }
#if defined(GPL_CODE)
    //
    // Why stop other CPU here? Because the kernel will happen "call stack overflow"
    // after finishing writing flash, so stop other CPU before restart kernel. If do
    // this before beginning writing flash, it'll cause client Browser has no upgrade's
    // status!
    //
    if (!is_cfe_write)
        stopOtherCpu();
#endif

    return sts;
}

/*******************************************************************************
 * SP functions
 * SP = ScratchPad, one or more sectors in the flash which user apps can
 * store small bits of data referenced by a small tag at the beginning.
 * kerSysScratchPadSet() and kerSysScratchPadCLearAll() must be protected by
 * a mutex because they do read/modify/writes to the flash sector(s).
 * kerSysScratchPadGet() and KerSysScratchPadList() do not need to acquire
 * the mutex, however, I acquire the mutex anyways just to make this interface
 * symmetrical.  High performance and concurrency is not needed on this path.
 *
 *******************************************************************************/

// get scratch pad data into *** pTempBuf *** which has to be released by the
//      caller!
// return: if pTempBuf != NULL, points to the data with the dataSize of the
//      buffer
// !NULL -- ok
// NULL  -- fail
static char *getScratchPad(int len)
{
    /* Root file system is on a writable NAND flash.  Read scratch pad from
     * a file on the NAND flash.
     */
    char *ret = NULL;

    if( (ret = retriedKmalloc(len)) != NULL )
    {
        struct file *fp;
        mm_segment_t fs;

        memset(ret, 0x00, len);
        fp = filp_open(SCRATCH_PAD_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((int) fp->f_op->read(fp, (void *) ret, len, &fp->f_pos) <= 0)
                printk("Failed to read scratch pad from '%s'\n",
                    SCRATCH_PAD_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }
    }
    else
        printk("Could not allocate scratch pad memory.\n");

    return( ret );
}

// set scratch pad - write the scratch pad file
// return:
// 0 -- ok
// -1 -- fail
static int setScratchPad(char *buf, int len)
{
    return kerSysFsFileSet(SCRATCH_PAD_FILE_NAME, buf, len);
}

/*
 * get list of all keys/tokenID's in the scratch pad.
 * NOTE: memcpy work here -- not using copy_from/to_user
 *
 * return:
 *         greater than 0 means number of bytes copied to tokBuf,
 *         0 means fail,
 *         negative number means provided buffer is not big enough and the
 *         absolute value of the negative number is the number of bytes needed.
 */
int kerSysScratchPadList(char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pBuf = NULL;
    char *pShareBuf = NULL;
    char *startPtr = NULL;
    int usedLen;
    int tokenNameLen=0;
    int copiedLen=0;
    int needLen=0;
    int sts = 0;

    BCM_ASSERT_NOT_HAS_MUTEX_R(&spMutex, 0);

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL) {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks(fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            printk("could not getSharedBlks.\n");
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;  
    }

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0) 
    {
        printk("Scratch pad is not initialized.\n");
        retriedKfree(pShareBuf);
        mutex_unlock(&spMutex);
        return sts;
    }

    // Walk through all the tokens
    usedLen = sizeof(SP_HEADER);
    startPtr = pBuf + sizeof(SP_HEADER);
    pToken = (PSP_TOKEN) startPtr;

    while( pToken->tokenName[0] != '\0' && pToken->tokenLen > 0 &&
           ((usedLen + pToken->tokenLen) <= fInfo.flash_scratch_pad_length))
    {
        tokenNameLen = strlen(pToken->tokenName);
        needLen += tokenNameLen + 1;
        if (needLen <= bufLen)
        {
            strcpy(&tokBuf[copiedLen], pToken->tokenName);
            copiedLen += tokenNameLen + 1;
        }

        usedLen += ((pToken->tokenLen + 0x03) & ~0x03);
        startPtr += sizeof(SP_TOKEN) + ((pToken->tokenLen + 0x03) & ~0x03);
        pToken = (PSP_TOKEN) startPtr;
    }

    if ( needLen > bufLen )
    {
        // User may purposely pass in a 0 length buffer just to get
        // the size, so don't log this as an error.
        sts = needLen * (-1);
    }
    else
    {
        sts = copiedLen;
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;
}

/*
 * get sp data.  NOTE: memcpy work here -- not using copy_from/to_user
 * return:
 *         greater than 0 means number of bytes copied to tokBuf,
 *         0 means fail,
 *         negative number means provided buffer is not big enough and the
 *         absolute value of the negative number is the number of bytes needed.
 */
int kerSysScratchPadGet(char *tokenId, char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pBuf = NULL;
    char *pShareBuf = NULL;
    char *startPtr = NULL;
    int usedLen;
    int sts = 0;

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL) {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks(fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            printk("could not getSharedBlks.\n");
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;
    }

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0) 
    {
        printk("Scratch pad is not initialized.\n");
        retriedKfree(pShareBuf);
        mutex_unlock(&spMutex);
        return sts;
    }

    // search for the token
    usedLen = sizeof(SP_HEADER);
    startPtr = pBuf + sizeof(SP_HEADER);
    pToken = (PSP_TOKEN) startPtr;
    while( pToken->tokenName[0] != '\0' && pToken->tokenLen > 0 &&
        pToken->tokenLen < fInfo.flash_scratch_pad_length &&
        usedLen < fInfo.flash_scratch_pad_length )
    {

        if (strncmp(pToken->tokenName, tokenId, TOKEN_NAME_LEN) == 0)
        {
            if ( pToken->tokenLen > bufLen )
            {
               // User may purposely pass in a 0 length buffer just to get
               // the size, so don't log this as an error.
               // printk("The length %d of token %s is greater than buffer len %d.\n", pToken->tokenLen, pToken->tokenName, bufLen);
                sts = pToken->tokenLen * (-1);
            }
            else
            {
                memcpy(tokBuf, startPtr + sizeof(SP_TOKEN), pToken->tokenLen);
                sts = pToken->tokenLen;
            }
            break;
        }

        usedLen += ((pToken->tokenLen + 0x03) & ~0x03);
        startPtr += sizeof(SP_TOKEN) + ((pToken->tokenLen + 0x03) & ~0x03);
        pToken = (PSP_TOKEN) startPtr;
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;
}

// set sp.  NOTE: memcpy work here -- not using copy_from/to_user
// return:
//  0 - ok
//  -1 - fail
int kerSysScratchPadSet(char *tokenId, char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pShareBuf = NULL;
    char *pBuf = NULL;
    SP_HEADER SPHead;
    SP_TOKEN SPToken;
    char *curPtr;
    int sts = -1;

    if( bufLen >= fInfo.flash_scratch_pad_length - sizeof(SP_HEADER) -
        sizeof(SP_TOKEN) )
    {
        printk("Scratch pad overflow by %d bytes.  Information not saved.\n",
            bufLen  - fInfo.flash_scratch_pad_length - sizeof(SP_HEADER) -
            sizeof(SP_TOKEN));
        return sts;
    }

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL)
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;  
    }

    // form header info.
    memset((char *)&SPHead, 0, sizeof(SP_HEADER));
    memcpy(SPHead.SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN);
    SPHead.SPVersion = SP_VERSION;

    // form token info.
    memset((char*)&SPToken, 0, sizeof(SP_TOKEN));
    strncpy(SPToken.tokenName, tokenId, TOKEN_NAME_LEN - 1);
    SPToken.tokenLen = bufLen;

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0)
    {
        // new sp, so just flash the token
        printk("No scratch pad found.  Initialize scratch pad...\n");
        memcpy(pBuf, (char *)&SPHead, sizeof(SP_HEADER));
        curPtr = pBuf + sizeof(SP_HEADER);
        memcpy(curPtr, (char *)&SPToken, sizeof(SP_TOKEN));
        curPtr += sizeof(SP_TOKEN);
        if( tokBuf )
            memcpy(curPtr, tokBuf, bufLen);
    }
    else  
    {
        int putAtEnd = 1;
        int curLen;
        int usedLen;
        int skipLen;

        /* Calculate the used length. */
        usedLen = sizeof(SP_HEADER);
        curPtr = pBuf + sizeof(SP_HEADER);
        pToken = (PSP_TOKEN) curPtr;
        skipLen = (pToken->tokenLen + 0x03) & ~0x03;
        while( pToken->tokenName[0] >= 'A' && pToken->tokenName[0] <= 'z' &&
            strlen(pToken->tokenName) < TOKEN_NAME_LEN &&
            pToken->tokenLen > 0 &&
            pToken->tokenLen < fInfo.flash_scratch_pad_length &&
            usedLen < fInfo.flash_scratch_pad_length )
        {
            usedLen += sizeof(SP_TOKEN) + skipLen;
            curPtr += sizeof(SP_TOKEN) + skipLen;
            pToken = (PSP_TOKEN) curPtr;
            skipLen = (pToken->tokenLen + 0x03) & ~0x03;
        }

        if( usedLen + SPToken.tokenLen + sizeof(SP_TOKEN) >
            fInfo.flash_scratch_pad_length )
        {
            printk("Scratch pad overflow by %d bytes.  Information not saved.\n",
                (usedLen + SPToken.tokenLen + sizeof(SP_TOKEN)) -
                fInfo.flash_scratch_pad_length);
            mutex_unlock(&spMutex);
            return sts;
        }

        curPtr = pBuf + sizeof(SP_HEADER);
        curLen = sizeof(SP_HEADER);
        while( curLen < usedLen )
        {
            pToken = (PSP_TOKEN) curPtr;
            skipLen = (pToken->tokenLen + 0x03) & ~0x03;
            if (strncmp(pToken->tokenName, tokenId, TOKEN_NAME_LEN) == 0)
            {
                // The token id already exists.
                if( tokBuf && pToken->tokenLen == bufLen )
                {
                    // The length of the new data and the existing data is the
                    // same.  Overwrite the existing data.
                    memcpy((curPtr+sizeof(SP_TOKEN)), tokBuf, bufLen);
                    putAtEnd = 0;
                }
                else
                {
                    // The length of the new data and the existing data is
                    // different.  Shift the rest of the scratch pad to this
                    // token's location and put this token's data at the end.
                    char *nextPtr = curPtr + sizeof(SP_TOKEN) + skipLen;
                    int copyLen = usedLen - (curLen+sizeof(SP_TOKEN) + skipLen);
                    memcpy( curPtr, nextPtr, copyLen );
                    memset( curPtr + copyLen, 0x00, 
                        fInfo.flash_scratch_pad_length - (curLen + copyLen) );
                    usedLen -= sizeof(SP_TOKEN) + skipLen;
                }
                break;
            }

            // get next token
            curPtr += sizeof(SP_TOKEN) + skipLen;
            curLen += sizeof(SP_TOKEN) + skipLen;
        } // end while

        if( putAtEnd )
        {
            if( tokBuf )
            {
                memcpy( pBuf + usedLen, &SPToken, sizeof(SP_TOKEN) );
                memcpy( pBuf + usedLen + sizeof(SP_TOKEN), tokBuf, bufLen );
            }
            memcpy( pBuf, &SPHead, sizeof(SP_HEADER) );
        }

    } // else if not new sp

    if( bootFromNand )
        sts = setScratchPad(pShareBuf, fInfo.flash_scratch_pad_length);
    else
        sts = setSharedBlks(fInfo.flash_scratch_pad_start_blk, 
            fInfo.flash_scratch_pad_number_blk, pShareBuf);
    
    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;

    
}

#if defined (CONFIG_BCM96838) || defined (CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
EXPORT_SYMBOL(kerSysScratchPadGet);
EXPORT_SYMBOL(kerSysScratchPadSet);
#endif

#if defined(GPL_CODE)
int kerClearScratchPad(int blk_size)
{
    char buf[256];

    memset(buf, 0, 256);

#if defined(GPL_CODE_CONFIG_JFFS)
    if(bootFromNand)
#else
    if(!bootFromNand)
#endif
    {
        kerSysPersistentSet( buf, 256, 0 );
#ifdef SUPPORT_BACKUP_PSI
        kerSysBackupPsiSet( buf, 256, 0 );
#endif

        kerSysScratchPadClearAll();
#if defined(GPL_CODE) && defined(GPL_CODE_TWO_IN_ONE_FIRMWARE)
        unsigned char boardid[NVRAM_BOARD_ID_STRING_LEN] = {0};

        kerSysGetBoardID(boardid);

        if(strstr(boardid, "C2000") || strstr(boardid, "C1900"))
           restoreDatapump(0);
        else if (strstr(boardid, "C1000"))
           restoreDatapump(2);

#else  // AEI_CONFIG_JFFS
#if defined(GPL_CODE_Q2000H)
        restoreDatapump(2);
#else
        restoreDatapump(0);
#endif
#endif
    }
}
#endif

// wipe out the scratchPad
// return:
//  0 - ok
//  -1 - fail
int kerSysScratchPadClearAll(void)
{ 
    int sts = -1;
    char *pShareBuf = NULL;
    int j ;
    int usedBlkSize = 0;

#if defined(GPL_CODE_UPGRADE_HISTORY_SPAD)
	/* get upgradeHistroy */
    char *uph;
	int upgradeRet = 0;
#if defined(GPL_CODE_CONFIG_JFFS)
	if(bootFromNand)
#else
	if(!bootFromNand)
#endif
    {
        if (NULL != (uph = kmalloc(1024, GFP_KERNEL)) )
        {
            upgradeRet = kerSysScratchPadGet("upgradeHistory", uph, 1024);
        }
        else
        {
            printk("failed to allocate 1024 bytes for upgrade history\n");
        }
    }

#endif

#if defined(GPL_CODE)
	typedef struct {
	    char date[128];
	    char time[128];
	}CfgSaveTime;
	CfgSaveTime savedTime;
	int ret = 0;
#ifdef GPL_CODE_FRONTIER_V2210
    memset(&savedTime,0x00,sizeof(CfgSaveTime));
#endif
	ret=kerSysScratchPadGet("CfgSaveState", &savedTime, sizeof(CfgSaveTime) );
#endif

    // printk ("kerSysScratchPadClearAll.... \n") ;
    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL)
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        memset(pShareBuf, 0x00, fInfo.flash_scratch_pad_length);

#if defined(GPL_CODE)
	sts = setScratchPad(pShareBuf, fInfo.flash_scratch_pad_length);
#else
        setScratchPad(pShareBuf, fInfo.flash_scratch_pad_length);
#endif
    }
    else
    {
        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        if (fInfo.flash_scratch_pad_number_blk == 1)
            memset(pShareBuf + fInfo.flash_scratch_pad_blk_offset, 0x00, fInfo.flash_scratch_pad_length) ;
        else
        {
            for (j = fInfo.flash_scratch_pad_start_blk;
                j < (fInfo.flash_scratch_pad_start_blk + fInfo.flash_scratch_pad_number_blk);
                j++)
            {
                usedBlkSize += flash_get_sector_size((unsigned short) j);
            }

            memset(pShareBuf, 0x00, usedBlkSize) ;
        }

        sts = setSharedBlks(fInfo.flash_scratch_pad_start_blk,    
            fInfo.flash_scratch_pad_number_blk,  pShareBuf);
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

#if defined(GPL_CODE_UPGRADE_HISTORY_SPAD)
    /* save upgradeHistory back to scratch */
#if defined(GPL_CODE_CONFIG_JFFS)
	if(bootFromNand)
#else
    if(!bootFromNand)
#endif
	{
        if ( upgradeRet && uph )
           kerSysScratchPadSet("upgradeHistory", uph, 1024);

        if ( uph )  kfree(uph);
    }
#endif

#if defined(GPL_CODE)
	if ( ret )
	{
	  kerSysScratchPadSet("CfgSaveState", &savedTime, sizeof(CfgSaveTime) );
	}
#endif

    //printk ("kerSysScratchPadClearAll Done.... \n") ;
    return sts;
}

int kerSysFlashSizeGet(void)
{
    int ret = 0;

    if( bootFromNand )
    {
        struct mtd_info *mtd;

        if( (mtd = get_mtd_device_nm("image")) != NULL )
        {
            ret = mtd->size;
            put_mtd_device(mtd);
        }
    }
    else
        ret = flash_get_total_size();

    return ret;
}

/***********************************************************************
 * Function Name: writeBootImageState
 * Description  : Persistently sets the state of an image update.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int writeBootImageState( int currState, int newState )
{
    int ret = -1;

    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;

        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) != NULL )
        {
            PSP_HEADER pHdr = (PSP_HEADER) pShareBuf;
            unsigned long *pBootImgState=(unsigned long *)&pHdr->NvramData2[0];

            /* The boot image state is stored as a word in flash memory where
             * the most significant three bytes are a "magic number" and the
             * least significant byte is the state constant.
             */
            if( (*pBootImgState & 0xffffff00) == (BLPARMS_MAGIC & 0xffffff00) )
            {
                *pBootImgState &= ~0x000000ff;
                *pBootImgState |= newState;

                ret = setSharedBlks(fInfo.flash_scratch_pad_start_blk,    
                    fInfo.flash_scratch_pad_number_blk,  pShareBuf);
            }

            retriedKfree(pShareBuf);
        }
    }
    else
    {
        mm_segment_t fs;
        struct file *fp;

        /* NAND flash */
        char state_name[] = "/data/" NAND_BOOT_STATE_FILE_NAME;

        fs = get_fs();
        set_fs(get_ds());

        /* Remove old file:
         * This must happen before a new file is created so that the new file-
         * name will have a higher version in the FS, this is also the reason
         * why renaming might not work well (higher version might be exist as
         * deleted in FS) */
        if( currState != -1 )
        {
            state_name[strlen(state_name) - 1] = currState;
            sys_unlink(state_name);
        }

        /* Create new state file name. */
        state_name[strlen(state_name) - 1] = newState;
        fp = filp_open(state_name, O_RDWR | O_TRUNC | O_CREAT,
                S_IRUSR | S_IWUSR);

        if (!IS_ERR(fp))
        {
            fp->f_pos = 0;
            fp->f_op->write(fp, (void *) "boot state\n",
                    strlen("boot state\n"), &fp->f_pos);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
            vfs_fsync(fp, 0);
#else
            vfs_fsync(fp, fp->f_path.dentry, 0);
#endif
            filp_close(fp, NULL);
        }
        else
            printk("Unable to open '%s'.\n", state_name);

        set_fs(fs);
        ret = 0;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: readBootImageState
 * Description  : Reads the current boot image state from flash memory.
 * Returns      : state constant or -1 for failure
 ***********************************************************************/
static int readBootImageState( void )
{
    int ret = -1;

    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;

        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) != NULL )
        {
            PSP_HEADER pHdr = (PSP_HEADER) pShareBuf;
            unsigned long *pBootImgState=(unsigned long *)&pHdr->NvramData2[0];

            /* The boot image state is stored as a word in flash memory where
             * the most significant three bytes are a "magic number" and the
             * least significant byte is the state constant.
             */
            if( (*pBootImgState & 0xffffff00) == (BLPARMS_MAGIC & 0xffffff00) )
            {
                ret = *pBootImgState & 0x000000ff;
            }

            retriedKfree(pShareBuf);
        }
    }
    else
    {
        /* NAND flash */
        int i;
        char states[] = {BOOT_SET_NEW_IMAGE, BOOT_SET_OLD_IMAGE,
            BOOT_SET_NEW_IMAGE_ONCE};
        char boot_state_name[] = "/data/" NAND_BOOT_STATE_FILE_NAME;

        /* The boot state is stored as the last character of a file name on
         * the data partition, /data/boot_state_X, where X is
         * BOOT_SET_NEW_IMAGE, BOOT_SET_OLD_IMAGE or BOOT_SET_NEW_IMAGE_ONCE.
         */
        for( i = 0; i < sizeof(states); i++ )
        {
            struct file *fp;
            boot_state_name[strlen(boot_state_name) - 1] = states[i];
            fp = filp_open(boot_state_name, O_RDONLY, 0);
            if (!IS_ERR(fp) )
            {
                filp_close(fp, NULL);

                ret = (int) states[i];
                break;
            }
        }

        if( ret == -1 && writeBootImageState( -1, BOOT_SET_NEW_IMAGE ) == 0 )
            ret = BOOT_SET_NEW_IMAGE;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: dirent_rename
 * Description  : Renames file oldname to newname by parsing NAND flash
 *                blocks with JFFS2 file system entries.  When the JFFS2
 *                directory entry inode for oldname is found, it is modified
 *                for newname.  This differs from a file system rename
 *                operation that creates a new directory entry with the same
 *                inode value and greater version number.  The benefit of
 *                this method is that by having only one directory entry
 *                inode, the CFE ROM can stop at the first occurance which
 *                speeds up the boot by not having to search the entire file
 *                system partition.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int dirent_rename(char *oldname, char *newname)
{
    int ret = -1;
    struct mtd_info *mtd;
    unsigned char *buf;

    mtd = get_mtd_device_nm("bootfs_update");

    if( IS_ERR_OR_NULL(mtd) )
        mtd = get_mtd_device_nm("rootfs_update"); 

    buf = (mtd) ? retriedKmalloc(mtd->erasesize) : NULL;

    if( mtd && buf && strlen(oldname) == strlen(newname) )
    {
        int namelen = strlen(oldname);
        int blk, done, retlen;

        /* Read NAND flash blocks in the rootfs_update JFFS2 file system
         * partition to search for a JFFS2 directory entry for the oldname
         * file.
         */
        for(blk = 0, done = 0; blk < mtd->size && !done; blk += mtd->erasesize)
        {
            if(mtd_read(mtd, blk, mtd->erasesize, &retlen, buf) == 0)
            {
                struct jffs2_raw_dirent *pdir;
                unsigned char *p = buf;

                while( p < buf + mtd->erasesize )
                {
                    pdir = (struct jffs2_raw_dirent *) p;
                    if( je16_to_cpu(pdir->magic) == JFFS2_MAGIC_BITMASK )
                    {
                        if( je16_to_cpu(pdir->nodetype) ==
                                JFFS2_NODETYPE_DIRENT &&
                            !memcmp(oldname, pdir->name, namelen) &&
                            je32_to_cpu(pdir->ino) != 0 )
                        {
                            /* Found the oldname directory entry.  Change the
                             * name to newname.
                             */
                            memcpy(pdir->name, newname, namelen);
                            je32_to_cpu(pdir->name_crc) = crc32(0, pdir->name,
                                (unsigned int) namelen);

                            /* Write the modified block back to NAND flash. */
                            if(nandEraseBlk( mtd, blk ) == 0)
                            {
                                if( nandWriteBlk(mtd, blk, mtd->erasesize, buf, TRUE) == 0 )
                                {
                                    ret = 0;
                                }
                            }

                            done = 1;
                            break;
                        }

                        p += (je32_to_cpu(pdir->totlen) + 0x03) & ~0x03;
                    }
                    else
                        break;
                }
            }
        }
    }
    else
        printk("%s: Error renaming file\n", __FUNCTION__);

    if( mtd )
        put_mtd_device(mtd);

    if( buf )
        retriedKfree(buf);

    return( ret );
}

/***********************************************************************
 * Function Name: updateSequenceNumber
 * Description  : updates the sequence number on the specified partition
 *                to be the highest.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int updateSequenceNumber(int incSeqNumPart, int seqPart1, int seqPart2)
{
    int ret = -1;

    mutex_lock(&flashImageMutex);
    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;
        PFILE_TAG pTag;
        int blk;

        pTag = kerSysUpdateTagSequenceNumber(incSeqNumPart);
        blk = *(int *) (pTag + 1);

        if ((pShareBuf = getSharedBlks(blk, 1)) != NULL)
        {
            memcpy(pShareBuf, pTag, sizeof(FILE_TAG));
            setSharedBlks(blk, 1, pShareBuf);
            retriedKfree(pShareBuf);
        }
    }
    else
    {
        /* NAND flash */

        /* The sequence number on NAND flash is updated by renaming file
         * cferam.XXX where XXX is the sequence number. The rootfs partition
         * is usually read-only. Therefore, only the cferam.XXX file on the
         * rootfs_update partiton is modified. Increase or decrase the
         * sequence number on the rootfs_update partition so the desired
         * partition boots.
         */
        int seq = -1;
        unsigned long rootfs_ofs = 0xff;

        kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs);

        if(rootfs_ofs == inMemNvramData.ulNandPartOfsKb[NP_ROOTFS_1] )
        {
            /* Partition 1 is the booted partition. */
            if( incSeqNumPart == 2 )
            {
                /* Increase sequence number in rootfs_update partition. */
                if( seqPart1 >= seqPart2 )
                    seq = seqPart1 + 1;
            }
            else
            {
                /* Decrease sequence number in rootfs_update partition. */
                if( seqPart2 >= seqPart1 && seqPart1 != 0 )
                    seq = seqPart1 - 1;
            }
        }
        else
        {
            /* Partition 2 is the booted partition. */
            if( incSeqNumPart == 1 )
            {
                /* Increase sequence number in rootfs_update partition. */
                if( seqPart2 >= seqPart1 )
                    seq = seqPart2 + 1;
            }
            else
            {
                /* Decrease sequence number in rootfs_update partition. */
                if( seqPart1 >= seqPart2 && seqPart2 != 0 )
                    seq = seqPart2 - 1;
            }
        }

        if( seq != -1 )
        {
            struct mtd_info *mtd;
            char mtd_part[32];

            /* Find the sequence number of the non-booted partition. */
            mm_segment_t fs;

            aei_mtd_unlock();
            fs = get_fs();
            set_fs(get_ds());

            mtd = get_mtd_device_nm("bootfs_update");
            if( !IS_ERR_OR_NULL(mtd) )
            {
                strcpy(mtd_part, "mtd:bootfs_update");
                put_mtd_device(mtd);
            }
            else
                strcpy(mtd_part, "mtd:rootfs_update");


            if( sys_mount(mtd_part, "/mnt", "jffs2", 0, NULL) == 0 )
            {
                char fname[] = NAND_CFE_RAM_NAME;
                char cferam_old[32], cferam_new[32], cferam_fmt[32]; 
                int i;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
                /* If full secure boot is in play, the CFE RAM file is the encrypted version */
		int boot_secure = otp_is_boot_secure();
                if (boot_secure)
                    strcpy(fname, NAND_CFE_RAM_SECBT_NAME);
#endif
                fname[strlen(fname) - 3] = '\0'; /* remove last three chars */
                strcpy(cferam_fmt, "/mnt/");
                strcat(cferam_fmt, fname);
                strcat(cferam_fmt, "%3.3d");

                for( i = 0; i < 999; i++ )
                {
                    struct file *fp;
                    sprintf(cferam_old, cferam_fmt, i);
                    fp = filp_open(cferam_old, O_RDONLY, 0);
                    if (!IS_ERR(fp) )
                    {
                        filp_close(fp, NULL);

                        /* Change the boot sequence number in the rootfs_update
                         * partition by renaming file cferam.XXX where XXX is
                         * a sequence number.
                         */
                        sprintf(cferam_new, cferam_fmt, seq);
                        if( NAND_FULL_PARTITION_SEARCH )
                        {
                            sys_rename(cferam_old, cferam_new);
                            sys_umount("/mnt", 0);
                        }
                        else
                        {
                            sys_umount("/mnt", 0);
                            dirent_rename(cferam_old + strlen("/mnt/"),
                                cferam_new + strlen("/mnt/"));
                        }
                        break;
                    }
                }
            }
            set_fs(fs);
            aei_mtd_lock();
        }
    }
    mutex_unlock(&flashImageMutex);

    return( ret );
}

/***********************************************************************
 * Function Name: kerSysSetBootImageState
 * Description  : Persistently sets the state of an image update.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysSetBootImageState( int newState )
{
    int ret = -1;
    int incSeqNumPart = -1;
    int writeImageState = 0;
    int currState = readBootImageState();
    int seq1 = kerSysGetSequenceNumber(1);
    int seq2 = kerSysGetSequenceNumber(2);

    /* Update the image state persistently using "new image" and "old image"
     * states.  Convert "partition" states to "new image" state for
     * compatibility with the non-OMCI image update.
     */
    mutex_lock(&spMutex);
    switch(newState)
    {
    case BOOT_SET_PART1_IMAGE:
        if( seq1 != -1 )
        {
            if( seq1 < seq2 )
                incSeqNumPart = 1;
            newState = BOOT_SET_NEW_IMAGE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART2_IMAGE:
        if( seq2 != -1 )
        {
            if( seq2 < seq1 )
                incSeqNumPart = 2;
            newState = BOOT_SET_NEW_IMAGE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART1_IMAGE_ONCE:
        if( seq1 != -1 )
        {
            if( seq1 < seq2 )
                incSeqNumPart = 1;
            newState = BOOT_SET_NEW_IMAGE_ONCE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART2_IMAGE_ONCE:
        if( seq2 != -1 )
        {
            if( seq2 < seq1 )
                incSeqNumPart = 2;
            newState = BOOT_SET_NEW_IMAGE_ONCE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_OLD_IMAGE:
    case BOOT_SET_NEW_IMAGE:
    case BOOT_SET_NEW_IMAGE_ONCE:
        /* The boot image state is stored as a word in flash memory where
         * the most significant three bytes are a "magic number" and the
         * least significant byte is the state constant.
         */
        if( currState == newState )
        {
            ret = 0;
        }
        else
        {
            writeImageState = 1;

            if(newState==BOOT_SET_NEW_IMAGE && currState==BOOT_SET_OLD_IMAGE)
            {
                /* The old (previous) image is being set as the new
                 * (current) image. Make sequence number of the old
                 * image the highest sequence number in order for it
                 * to become the new image.
                 */
#if defined(GPL_CODE)
                incSeqNumPart = -1;
#else
                incSeqNumPart = 0;
#endif
            }
        }
        break;

    default:
        break;
    }

    if( writeImageState )
        ret = writeBootImageState(currState, newState);

    mutex_unlock(&spMutex);

    if( incSeqNumPart != -1 )
        updateSequenceNumber(incSeqNumPart, seq1, seq2);

    return( ret );
}

/***********************************************************************
 * Function Name: kerSysGetBootImageState
 * Description  : Gets the state of an image update from flash.
 * Returns      : state constant or -1 for failure
 ***********************************************************************/
int kerSysGetBootImageState( void )
{
    int ret = readBootImageState();

    if( ret != -1 )
    {
        int seq1 = kerSysGetSequenceNumber(1);
        int seq2 = kerSysGetSequenceNumber(2);

        switch(ret)
        {
        case BOOT_SET_NEW_IMAGE:
            if( seq1 == -1 || seq1 < seq2)
                ret = BOOT_SET_PART2_IMAGE;
            else
                ret = BOOT_SET_PART1_IMAGE;
            break;

        case BOOT_SET_NEW_IMAGE_ONCE:
            if( seq1 == -1 || seq1 < seq2)
                ret = BOOT_SET_PART2_IMAGE_ONCE;
            else
                ret = BOOT_SET_PART1_IMAGE_ONCE;
            break;

        case BOOT_SET_OLD_IMAGE:
            if( seq1 == -1 || seq1 > seq2)
                ret = BOOT_SET_PART2_IMAGE;
            else
                ret = BOOT_SET_PART1_IMAGE;
            break;

        default:
            ret = -1;
            break;
        }
    }

    return( ret );
}


/***********************************************************************
 * Function Name: kerSysSetOpticalPowerValues
 * Description  : Saves optical power values to flash that are obtained
 *                during the  manufacturing process. These values are
 *                stored in NVRAM_DATA which should not be erased.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysSetOpticalPowerValues(UINT16 rxReading, UINT16 rxOffset, 
    UINT16 txReading)
{
    NVRAM_DATA *pNd;
    int ret = -1;
    BCM_ASSERT_NOT_HAS_MUTEX_C(&flashImageMutex);
    if ((pNd = kmalloc(NVRAM_LENGTH, GFP_KERNEL)) != NULL)
    {
       memcpy((char *)pNd, inMemNvramData_buf, NVRAM_LENGTH);

       pNd->opticRxPwrReading = rxReading;
       pNd->opticRxPwrOffset  = rxOffset;
       pNd->opticTxPwrReading = txReading;

       pNd->ulCheckSum = 0;
       pNd->ulCheckSum = crc32(CRC32_INIT_VALUE, pNd, sizeof(NVRAM_DATA));

       mutex_lock(&flashImageMutex);
       ret = kerSysNvRamSet((char *)pNd, NVRAM_LENGTH, 0);
       mutex_unlock(&flashImageMutex);
       kfree(pNd);
    }
    return(ret);

#if 0
    NVRAM_DATA nd;

    kerSysNvRamGet((char *) &nd, sizeof(nd), 0);

    nd.opticRxPwrReading = rxReading;
    nd.opticRxPwrOffset  = rxOffset;
    nd.opticTxPwrReading = txReading;
    
    nd.ulCheckSum = 0;
    nd.ulCheckSum = crc32(CRC32_INIT_VALUE, &nd, sizeof(NVRAM_DATA));

    return(kerSysNvRamSet((char *) &nd, sizeof(nd), 0));

#endif

}

/***********************************************************************
 * Function Name: kerSysGetOpticalPowerValues
 * Description  : Retrieves optical power values from flash that were
 *                saved during the manufacturing process.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysGetOpticalPowerValues(UINT16 *prxReading, UINT16 *prxOffset, 
    UINT16 *ptxReading)
{
#if 0
    NVRAM_DATA nd;

    kerSysNvRamGet((char *) &nd, sizeof(nd), 0);

    *prxReading = nd.opticRxPwrReading;
    *prxOffset  = nd.opticRxPwrOffset;
    *ptxReading = nd.opticTxPwrReading;
#else
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);

    *prxReading = inMemNvramData.opticRxPwrReading;
    *prxOffset  = inMemNvramData.opticRxPwrOffset;
    *ptxReading = inMemNvramData.opticTxPwrReading;

    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
#endif

    return(0);
}


#if !defined(CONFIG_BRCM_IKOS)

int kerSysEraseFlash(unsigned long eraseaddr, unsigned long len)
{
    int blk;
    int bgnBlk = flash_get_blk(eraseaddr);
    int endBlk = flash_get_blk(eraseaddr + len);
    unsigned long bgnAddr = (unsigned long) flash_get_memptr(bgnBlk);
    unsigned long endAddr = (unsigned long) flash_get_memptr(endBlk);

#ifdef DEBUG_FLASH
    printk("kerSysEraseFlash blk[%d] eraseaddr[0x%08x] len[%lu]\n",
    bgnBlk, (int)eraseaddr, len);
#endif

    if ( bgnAddr != eraseaddr)
    {
       printk("ERROR: kerSysEraseFlash eraseaddr[0x%08x]"
              " != first block start[0x%08x]\n",
              (int)eraseaddr, (int)bgnAddr);
        return (len);
    }

    if ( (endAddr - bgnAddr) != len)
    {
        printk("ERROR: kerSysEraseFlash eraseaddr[0x%08x] + len[%lu]"
               " != last+1 block start[0x%08x]\n",
               (int)eraseaddr, len, (int) endAddr);
        return (len);
    }

    for (blk=bgnBlk; blk<endBlk; blk++)
        flash_sector_erase_int(blk);

    return 0;
}



unsigned long kerSysReadFromFlash( void *toaddr, unsigned long fromaddr,
    unsigned long len )
{
    int blk, offset, bytesRead;
    unsigned long blk_start;
    char * trailbyte = (char*) NULL;
    char val[2];

    blk = flash_get_blk((int)fromaddr); /* sector in which fromaddr falls */
    blk_start = (unsigned long)flash_get_memptr(blk); /* sector start address */
    offset = (int)(fromaddr - blk_start); /* offset into sector */

#ifdef DEBUG_FLASH
    printk("kerSysReadFromFlash blk[%d] fromaddr[0x%08x]\n",
           blk, (int)fromaddr);
#endif

    bytesRead = 0;

        /* cfiflash : hardcoded for bankwidths of 2 bytes. */
    if ( offset & 1 )   /* toaddr is not 2 byte aligned */
    {
        flash_read_buf(blk, offset-1, val, 2);
        *((char*)toaddr) = val[1];

        toaddr = (void*)((char*)toaddr+1);
        fromaddr += 1;
        len -= 1;
        bytesRead = 1;

        /* if len is 0 we could return here, avoid this if */

        /* recompute blk and offset, using new fromaddr */
        blk = flash_get_blk(fromaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(fromaddr - blk_start);
    }

        /* cfiflash : hardcoded for len of bankwidths multiples. */
    if ( len & 1 )
    {
        len -= 1;
        trailbyte = (char *)toaddr + len;
    }

        /* Both len and toaddr will be 2byte aligned */
    if ( len )
    {
       flash_read_buf(blk, offset, toaddr, len);
       bytesRead += len;
    }

        /* write trailing byte */
    if ( trailbyte != (char*) NULL )
    {
        fromaddr += len;
        blk = flash_get_blk(fromaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(fromaddr - blk_start);
        flash_read_buf(blk, offset, val, 2 );
        *trailbyte = val[0];
        bytesRead += 1;
    }

    return( bytesRead );
}

/*
 * Function: kerSysWriteToFlash
 *
 * Description:
 * This function assumes that the area of flash to be written was
 * previously erased. An explicit erase is therfore NOT needed 
 * prior to a write. This function ensures that the offset and len are
 * two byte multiple. [cfiflash hardcoded for bankwidth of 2 byte].
 *
 * Parameters:
 *      toaddr : destination flash memory address
 *      fromaddr: RAM memory address containing data to be written
 *      len : non zero bytes to be written
 * Return:
 *      FAILURE: number of bytes remaining to be written
 *      SUCCESS: 0 (all requested bytes were written)
 */
int kerSysWriteToFlash( unsigned long toaddr,
                        void * fromaddr, unsigned long len)
{
    int blk, offset, size, blk_size, bytesWritten;
    unsigned long blk_start;
    char * trailbyte = (char*) NULL;
    unsigned char val[2];

#ifdef DEBUG_FLASH
    printk("kerSysWriteToFlash flashAddr[0x%08x] fromaddr[0x%08x] len[%lu]\n",
    (int)toaddr, (int)fromaddr, len);
#endif

    blk = flash_get_blk(toaddr);    /* sector in which toaddr falls */
    blk_start = (unsigned long)flash_get_memptr(blk); /* sector start address */
    offset = (int)(toaddr - blk_start); /* offset into sector */

    /* cfiflash : hardcoded for bankwidths of 2 bytes. */
    if ( offset & 1 )   /* toaddr is not 2 byte aligned */
    {
        val[0] = 0xFF; // ignored
        val[1] = *((char *)fromaddr); /* write the first byte */
        bytesWritten = flash_write_buf(blk, offset-1, val, 2);
        if ( bytesWritten != 2 )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%lui>\n", len); 
#endif
           return len;
        }

        toaddr += 1;
        fromaddr = (void*)((char*)fromaddr+1);
        len -= 1;

    /* if len is 0 we could return bytesWritten, avoid this if */

    /* recompute blk and offset, using new toaddr */
        blk = flash_get_blk(toaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(toaddr - blk_start);
    }

    /* cfiflash : hardcoded for len of bankwidths multiples. */
    if ( len & 1 )
    {
    /* need to handle trailing byte seperately */
        len -= 1;
        trailbyte = (char *)fromaddr + len;
        toaddr += len;
    }

    /* Both len and toaddr will be 2byte aligned */
    while ( len > 0 )
    {
        blk_size = flash_get_sector_size(blk);
        if (FLASH_API_ERROR == blk_size) {
           return len;
        }
        size = blk_size - offset; /* space available in sector from offset */
        if ( size > len )
            size = len;

        bytesWritten = flash_write_buf(blk, offset, fromaddr, size); 
        if ( bytesWritten !=  size )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%lui>\n", 
               (len - bytesWritten + ((trailbyte == (char*)NULL)? 0 : 1)));
#endif
           return (len - bytesWritten + ((trailbyte == (char*)NULL)? 0 : 1));
        }

        fromaddr += size;
        len -= size;

        blk++;      /* Move to the next block */
        offset = 0; /* All further blocks will be written at offset 0 */
    }

    /* write trailing byte */
    if ( trailbyte != (char*) NULL )
    {
        blk = flash_get_blk(toaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(toaddr - blk_start);
        val[0] = *trailbyte; /* trailing byte */
        val[1] = 0xFF; // ignored
        bytesWritten = flash_write_buf(blk, offset, val, 2 );
        if ( bytesWritten != 2 )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%d>\n",1);
#endif
           return 1;
        }
    } 

    return len;
}
/*
 * Function: kerSysWriteToFlashREW
 * 
 * Description:
 * This function does not assume that the area of flash to be written was erased.
 * An explicit erase is therfore needed prior to a write.  
 * kerSysWriteToFlashREW uses a sector copy  algorithm. The first and last sectors
 * may need to be first read if they are not fully written. This is needed to
 * avoid the situation that there may be some valid data in the sector that does
 * not get overwritten, and would be erased.
 *
 * Due to run time costs for flash read, optimizations to read only that data
 * that will not be overwritten is introduced.
 *
 * Parameters:
 *  toaddr : destination flash memory address
 *  fromaddr: RAM memory address containing data to be written
 *  len : non zero bytes to be written
 * Return:
 *  FAILURE: number of bytes remaining to be written 
 *  SUCCESS: 0 (all requested bytes were written)
 *
 */
int kerSysWriteToFlashREW( unsigned long toaddr,
                        void * fromaddr, unsigned long len)
{
    int blk, offset, size, blk_size, bytesWritten;
    unsigned long sect_start;
    int mem_sz = 0;
    char * mem_p = (char*)NULL;

#ifdef DEBUG_FLASH
    printk("kerSysWriteToFlashREW flashAddr[0x%08x] fromaddr[0x%08x] len[%lu]\n",
    (int)toaddr, (int)fromaddr, len);
#endif

    blk = flash_get_blk( toaddr );
    sect_start = (unsigned long) flash_get_memptr(blk);
    offset = toaddr - sect_start;

    while ( len > 0 )
    {
        blk_size = flash_get_sector_size(blk);
        size = blk_size - offset; /* space available in sector from offset */

        /* bound size to remaining len in final block */
        if ( size > len )
            size = len;

        /* Entire blk written, no dirty data to read */
        if ( size == blk_size )
        {
            flash_sector_erase_int(blk);

            bytesWritten = flash_write_buf(blk, 0, fromaddr, blk_size);

            if ( bytesWritten != blk_size )
            {
                if ( mem_p != NULL )
                    retriedKfree(mem_p);
                return (len - bytesWritten);    /* FAILURE */
            }
        }
        else
        {
                /* Support for variable sized blocks, paranoia */
            if ( (mem_p != NULL) && (mem_sz < blk_size) )
            {
                retriedKfree(mem_p);    /* free previous temp buffer */
                mem_p = (char*)NULL;
            }

            if ( (mem_p == (char*)NULL)
              && ((mem_p = (char*)retriedKmalloc(blk_size)) == (char*)NULL) )
            {
                printk("\tERROR kerSysWriteToFlashREW fail to allocate memory\n");
                return len;
            }
            else
                mem_sz = blk_size;

            if ( offset ) /* First block */
            {
                if ( (offset + size) == blk_size)
                {
                   flash_read_buf(blk, 0, mem_p, offset);
                }
                else
                {  
                   /*
                    * Potential for future optimization:
                    * Should have read the begining and trailing portions
                    * of the block. If the len written is smaller than some
                    * break even point.
                    * For now read the entire block ... move on ...
                    */
                   flash_read_buf(blk, 0, mem_p, blk_size);
                }
            }
            else
            {
                /* Read the tail of the block which may contain dirty data*/
                flash_read_buf(blk, len, mem_p+len, blk_size-len );
            }

            flash_sector_erase_int(blk);

            memcpy(mem_p+offset, fromaddr, size); /* Rebuild block contents */

            bytesWritten = flash_write_buf(blk, 0, mem_p, blk_size);

            if ( bytesWritten != blk_size )
            {
                if ( mem_p != (char*)NULL )
                    retriedKfree(mem_p);
                return (len + (blk_size - size) - bytesWritten );
            }
        }

        /* take into consideration that size bytes were copied */
        fromaddr += size;
        toaddr += size;
        len -= size;

        blk++;          /* Move to the next block */
        offset = 0;     /* All further blocks will be written at offset 0 */

    }

    if ( mem_p != (char*)NULL )
        retriedKfree(mem_p);

    return ( len );
}

#else //!defined(CONFIG_BRCM_IKOS)
int kerSysEraseFlash(unsigned long eraseaddr, unsigned long len)
{
    return 0;
}

int kerSysWriteToFlash( unsigned long toaddr,
                        void * fromaddr, unsigned long len)
{
    return 0;
}

#endif

#ifdef GPL_CODE_NAND_CNT_128K
struct mtd_part {
	struct mtd_info mtd;
	struct mtd_info *master;
	uint64_t offset;
	int index;
	struct list_head list;
	int registered;
};

int AEI_kerSysGetFlashCNT( char __user *buf,char  __user *type)
{	
	unsigned int addr;
	unsigned int total = 0;
	int sts =-1;
    struct mtd_oob_ops ops;
    unsigned char oobbuf[64]={0,};
    unsigned char erase_cnt[4]={0,};
    unsigned int *p_count = NULL;
  	struct mtd_info *mtd;
    struct mtd_part *part=NULL;
    int real_addr;
	int blk;
	unsigned int i;
	int blk_cnt = 0;
   	char *buf_p = buf;
   	int act_type;
   
   	part_erase_info_t * part_erase_info_p = (part_erase_info_t *)buf_p;
   	blk_erase_info_t * blk_erase_info_idx = (blk_erase_info_t *)(buf_p + sizeof(part_erase_info_t));
   	
  	if (copy_from_user(&act_type, type, sizeof(int)))
		return -EFAULT;
			   	
	switch (act_type) 
    {
    	
    	case GET_FLASH_CNT_NVRAM:
    		mtd = get_mtd_device_nm("nvram");
    		strcpy(part_erase_info_p->part_name,"nvram");
    		if(mtd == -ENODEV)
			{
				printk("can not get mtd device [nvram]\n");
				return -1;
			}
    	break;
    	
    	case GET_FLASH_CNT_ROOTFS:
    		mtd = get_mtd_device_nm("rootfs");
    		strcpy(part_erase_info_p->part_name,"rootfs");
    		if(mtd == -ENODEV)
			{
				printk("can not get mtd device [rootfs]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_ROOTFS_UPDATE:
        	mtd = get_mtd_device_nm("rootfs_update");
        	strcpy(part_erase_info_p->part_name,"rootfs_update");
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [rootfs_update]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_DATA:
        	mtd = get_mtd_device_nm("data");
        	strcpy(part_erase_info_p->part_name,"data");
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [data]\n");
				return -1;
			}
        break;
               
        case GET_FLASH_CNT_TAG:
        	mtd = get_mtd_device_nm("tag");
        	strcpy(part_erase_info_p->part_name,"tag");
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [tag]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_TAG_UPDATE:
        	mtd = get_mtd_device_nm("tag_update");
        	strcpy(part_erase_info_p->part_name,"tag_update");
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [tag_update]\n");
				return -1;
			}
        break;
        
        default:
        	printk("wrong mtd partition name...\n");
			return -1;
        break;
                    		
    } 	

    part = container_of(mtd,struct mtd_part,mtd);

	p_count = (unsigned int *)erase_cnt;
	
    memset(oobbuf, 0x00, mtd->oobsize);
    memset(&ops, 0x00, sizeof(ops));
    ops.ooblen  = mtd->oobsize;
    ops.ooboffs = 0;
    ops.datbuf  = NULL;
    ops.oobbuf  = oobbuf;
    ops.len     = 0;
    ops.mode    = MTD_OPS_PLACE_OOB;
    	
    i = mtd->size;
    while(i!=0)
    {
    	i -= mtd->erasesize;
    	blk_cnt++;
    }

	part_erase_info_p->tlt_size = mtd->size;
    part_erase_info_p->blk_size = mtd->erasesize;
    part_erase_info_p->blk_cnt = blk_cnt;
	
	for(addr = 0 ; addr < (mtd->size) ; addr += mtd->erasesize)
	{		
		real_addr = addr + part->offset;
		blk       = real_addr>>mtd->erasesize_shift;

        sts = mtd->_read_oob(mtd, addr, &ops);

    	if(sts == 0)
    	{
			memcpy((char *)erase_cnt,(char *)oobbuf+16,4);
			blk_erase_info_idx->blk_no     = blk;
			blk_erase_info_idx->blk_addr   = real_addr;
			blk_erase_info_idx->erase_time = (*p_count)+1;				
		}
		else
			printk("read oob data failed\n");
			
		total +=  (*p_count)+1;
		blk_erase_info_idx++;
		
	}
	part_erase_info_p->tlt_erase_time = total;
	
	put_mtd_device(mtd);
			
	return 0;
		
}

int AEI_kerSysGetFlashCNTBufSize( char __user *len,char  __user *type)
{
	int sts =-1;
  	struct mtd_info *mtd;
    int real_addr;
	int blk;
	unsigned int i;
	int blk_cnt = 0;
	int total_len;
   	int act_type;
   	
	if (copy_from_user(&act_type, type, sizeof(int)))
		return -EFAULT;
			   	
	switch (act_type) 
    {
    	
    	case GET_FLASH_CNT_NVRAM:
    		mtd = get_mtd_device_nm("nvram");
    		
    		if(mtd == -ENODEV)
			{
				printk("can not get mtd device [nvram]\n");
				return -1;
			}
    	break;
    	
    	case GET_FLASH_CNT_ROOTFS:
    		mtd = get_mtd_device_nm("rootfs");
    		
    		if(mtd == -ENODEV)
			{
				printk("can not get mtd device [rootfs]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_ROOTFS_UPDATE:
        	mtd = get_mtd_device_nm("rootfs_update");
        	
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [rootfs_update]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_DATA:
        	mtd = get_mtd_device_nm("data");
        	
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [data]\n");
				return -1;
			}
        break;
               
        case GET_FLASH_CNT_TAG:
        	mtd = get_mtd_device_nm("tag");
        	
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [tag]\n");
				return -1;
			}
        break;
                
        case GET_FLASH_CNT_TAG_UPDATE:
        	mtd = get_mtd_device_nm("tag_update");
        	
        	if(mtd == -ENODEV)
			{
				printk("can not get mtd device [tag_update]\n");
				return -1;
			}
        break;
        
        default:
        	printk("wrong mtd partition name...\n");
			return -1;
        break;
                    		
    }
    	
    i = mtd->size;
    while(i!=0)
    {
    	i -= mtd->erasesize;
    	blk_cnt++;
    }
        
    total_len = sizeof(part_erase_info_t) + sizeof(blk_erase_info_t)*blk_cnt;
    
    if(copy_to_user(len, &total_len, sizeof(int)))
		return -EFAULT;
		
    put_mtd_device(mtd);
	
	return 0;
}



#endif
