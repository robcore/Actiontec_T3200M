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
/***************************************************************************
* File Name  : board.c
*
* Description: This file contains Linux character device driver entry
*              for the board related ioctl calls: flash, get free kernel
*              page and dump kernel memory, etc.
*
*
***************************************************************************/

/* Includes. */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/if.h>
#include <linux/pci.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/smp.h>
#include <linux/version.h>
#include <linux/reboot.h>
#include <linux/bcm_assert_locks.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/mtd/mtd.h>

#ifdef GPL_CODE_CONFIG_JFFS
#include <linux/syscalls.h>
#include <linux/jffs2.h>
#endif
#include <bcmnetlink.h>
#include <net/sock.h>
#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <board.h>
#if defined(CONFIG_BCM_6802_MoCA)
#include "./bbsi/bbsi.h"
#else
#include <spidevices.h>
#endif

#define  BCMTAG_EXE_USE
#include <bcmTag.h>
#include <boardparms.h>
#include <boardparms_voice.h>
#include <flash_api.h>
#include <bcm_intr.h>
#include <flash_common.h>
#include <shared_utils.h>
#include <bcm_pinmux.h>
#include <bcmpci.h>
#include <linux/bcm_log.h>
#include <bcmSpiRes.h>
//extern unsigned int flash_get_reserved_bytes_at_end(const FLASH_ADDR_INFO *fInfo);
#include <pushbutton.h>



#if defined(CONFIG_BCM96838)
#include "pmc_drv.h"
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#if defined(CONFIG_SMP)
#include <linux/cpu.h>
#endif
#include "pmc_dsl.h"
#include "pmc_apm.h"
#endif

#if defined(CONFIG_BCM963381)
#include "pmc_dsl.h"
#endif

#if defined(CONFIG_BCM_EXT_TIMER)
#include "bcm_ext_timer.h"
#endif

#if defined(BRCM_XDSL_DISTPOINT)
#include <dsldsp_operation.h>
#endif

/* Typedefs. */

/* SES Events flags */
#define SES_EVENT_BTN_PRESSED      0x00000001
#define SES_EVENT_BTN_AP           0x00000002
#define SES_EVENT_BTN_STA          0x00000004
#define SES_EVENTS                 SES_EVENT_BTN_PRESSED /*OR all values if any*/

/* SES Button press types */
#define SES_BTN_LEGACY             1
#define SES_BTN_AP                 2
#define SES_BTN_STA                3

#if defined (WIRELESS)
#define SES_LED_OFF                0
#define SES_LED_ON                 1
#define SES_LED_BLINK              2

#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)
#define WLAN_ONBOARD_SLOT	WLAN_ONCHIP_DEV_SLOT
#else
#define WLAN_ONBOARD_SLOT       1 /* Corresponds to IDSEL -- EBI_A11/PCI_AD12 */
#endif

#define BRCM_VENDOR_ID       0x14e4
#define BRCM_WLAN_DEVICE_IDS 0x4300
#define BRCM_WLAN_DEVICE_IDS_DEC 43

#define WLAN_ON   1
#define WLAN_OFF  0

#if defined(GPL_CODE)

#define WSC_PROC_IDLE         0
#define WSC_PROC_WAITING      1
#define WSC_PROC_SUCC         2
#define WSC_PROC_TIMEOUT      3
#define WSC_PROC_FAIL         4
#define WSC_PROC_M2_SENT      5
#define WSC_PROC_M7_SENT      6
#define WSC_PROC_MSG_DONE     7
#define WSC_PROC_PBC_OVERLAP  8
/*WPS SM State*/
#define WSC_EVENTS_PROC_START              2
#define WSC_EVENTS_PROC_IDLE               (WSC_PROC_IDLE + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_WAITING            (WSC_PROC_WAITING + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_SUCC               (WSC_PROC_SUCC  + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_TIMEOUT            (WSC_PROC_TIMEOUT + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_FAIL               (WSC_PROC_FAIL + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_M2_SENT            (WSC_PROC_M2_SENT + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_M7_SENT            (WSC_PROC_M7_SENT + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_MSG_DONE           (WSC_PROC_MSG_DONE + WSC_EVENTS_PROC_START)
#define WSC_EVENTS_PROC_PBC_OVERLAP        (WSC_PROC_PBC_OVERLAP + WSC_EVENTS_PROC_START)

#endif //GPL_CODE

#endif

#if defined(GPL_CODE)
extern int gPowerLedStatus;
#if defined(GPL_CODE_CHIP_D0_CHECK)
int  CPURevId=0;
char CPUSerialNumber[32]={0};
static void AEI_setExpSerialNumber();
#endif
#endif

#ifdef GPL_CODE_NAND_IMG_CHECK
int gSetWrongCRC = 0; //1=set wrong crc
#endif

#if defined(GPL_CODE)
AEI_BOARD_ID aeiBoardId = AEI_BOARD_UNKNOWN;
#endif

typedef struct
{
    unsigned long ulId;
    char chInUse;
    char chReserved[3];
} MAC_ADDR_INFO, *PMAC_ADDR_INFO;

typedef struct
{
    unsigned long ulNumMacAddrs;
    unsigned char ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN];
    MAC_ADDR_INFO MacAddrs[1];
} MAC_INFO, *PMAC_INFO;

typedef struct
{
    unsigned char gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN];
    unsigned char gponPassword[NVRAM_GPON_PASSWORD_LEN];
} GPON_INFO, *PGPON_INFO;

typedef struct
{
    unsigned long eventmask;
} BOARD_IOC, *PBOARD_IOC;


/*Dyinggasp callback*/
typedef void (*cb_dgasp_t)(void *arg);
typedef struct _CB_DGASP__LIST
{
    struct list_head list;
    char name[IFNAMSIZ];
    cb_dgasp_t cb_dgasp_fn;
    void *context;
}CB_DGASP_LIST , *PCB_DGASP_LIST;


/*watchdog timer callback*/
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
typedef int (*cb_watchdog_t)(void *arg);
typedef struct _CB_WDOG__LIST
{
    struct list_head list;
    char name[IFNAMSIZ];
    cb_watchdog_t cb_wd_fn;
    void *context;
}CB_WDOG_LIST , *PCB_WDOG_LIST;
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
/*SATA Test module callback */
int (*bcm_sata_test_ioctl_fn)(void *) =NULL; 
EXPORT_SYMBOL(bcm_sata_test_ioctl_fn);
#endif

/* Externs. */
extern struct file *fget_light(unsigned int fd, int *fput_needed);
extern unsigned long getMemorySize(void);
extern void __init boardLedInit(void);
extern void boardLedCtrl(BOARD_LED_NAME, BOARD_LED_STATE);
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
extern int otp_is_boot_secure(void);
extern int otp_is_btrm_boot(void);
#endif
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
extern int proc_show_rdp_mem(char *buf, char **start, off_t off, int cnt, int *eof, void *data);
#endif
/* Prototypes. */
static void set_mac_info( void );
static void set_gpon_info( void );
static int board_open( struct inode *inode, struct file *filp );
static ssize_t board_read(struct file *filp,  char __user *buffer, size_t count, loff_t *ppos);
static unsigned int board_poll(struct file *filp, struct poll_table_struct *wait);
static int board_release(struct inode *inode, struct file *filp);
static int board_ioctl( struct inode *inode, struct file *flip, unsigned int command, unsigned long arg );
#if defined(HAVE_UNLOCKED_IOCTL)
static long board_unlocked_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);
#endif

void btnHook_PlcUke(unsigned long timeInMs, unsigned long param);

static BOARD_IOC* borad_ioc_alloc(void);
static void borad_ioc_free(BOARD_IOC* board_ioc);

/*
 * flashImageMutex must be acquired for all write operations to
 * nvram, CFE, or fs+kernel image.  (cfe and nvram may share a sector).
 */
DEFINE_MUTEX(flashImageMutex);

static void writeNvramDataCrcLocked(PNVRAM_DATA pNvramData);
static PNVRAM_DATA readNvramData(void);

#if defined(HAVE_UNLOCKED_IOCTL)
static DEFINE_MUTEX(ioctlMutex);
#endif

/* DyingGasp function prototype */
#if  !defined(CONFIG_BCM960333)
static irqreturn_t kerSysDyingGaspIsr(int irq, void * dev_id);
#endif
static void __init kerSysInitDyingGaspHandler( void );
static void __exit kerSysDeinitDyingGaspHandler( void );
/* -DyingGasp function prototype - */
/* dgaspMutex Protects dyingGasp enable/disable functions */
/* also protects list add and delete, but is ignored during isr. */
static DEFINE_MUTEX(dgaspMutex);
static volatile int isDyingGaspTriggered = 0;

static int ConfigCs(BOARD_IOCTL_PARMS *parms);

#if defined(CONFIG_BCM96318)
static void __init kerSysInit6318Reset( void );
#endif

static irqreturn_t mocaHost_isr(int irq, void *dev_id);

static irqreturn_t  sesBtn_isr(int irq, void *dev_id);
static Bool         sesBtn_pressed(void);
static int  __init  sesBtn_mapIntr(int context);
static void __init  ses_board_init(void);
static void __exit  ses_board_deinit(void);

#if defined (WIRELESS)
static unsigned int sesBtn_poll(struct file *file, struct poll_table_struct *wait);
static ssize_t sesBtn_read(struct file *file,  char __user *buffer, size_t count, loff_t *ppos);
static void __init sesLed_mapGpio(void);
static void sesLed_ctrl(int action);
#if defined(GPL_CODE)
static void AEI_wlanLed_ctrl(int action);
#endif
#if defined(GPL_CODE_VOIP_LED)
static void AEI_VoipLed_ctrl(char *action);
#endif
static void __init kerSysScreenPciDevices(void);
static void kerSetWirelessPD(int state);

/* This spinlock is used to avoid race conditions caused by the
 * non-atomic test-and-set of sesBtn_active in sesBtn_read */
static DEFINE_SPINLOCK(sesBtn_newapi_spinlock);
#endif

#if defined CONFIG_BCM963138 || defined CONFIG_BCM963148
static void __init  nfc_board_init(void);
EXPORT_SYMBOL(BpGetNfcExtIntr);
EXPORT_SYMBOL(BpGetNfcPowerGpio);
EXPORT_SYMBOL(BpGetNfcWakeGpio);
EXPORT_SYMBOL(BpGetBitbangSclGpio);
EXPORT_SYMBOL(BpGetBitbangSdaGpio);
#endif

#if defined(GPL_CODE_DOWNGRADE_NVRAM_ADJUST)
static UBOOL8 AEI_DownGrade_AdjustNVRAM(void);
static UBOOL8 AEI_IsDownGrade_FromSDK12To6(char* string);
#endif
static void str_to_num(char* in, char *out, int len);
static int add_proc_files(void);
static int del_proc_files(void);
static int proc_get_param(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_set_param(struct file *f, const char *buf, unsigned long cnt, void *data);
static int proc_set_led(struct file *f, const char *buf, unsigned long cnt, void *data);

static irqreturn_t reset_isr(int irq, void *dev_id);

#if defined(GPL_CODE)
 #define RESET_HOLD_TIME		10
 #define FACTORY_HOLD_TIME	20
#elif defined(GPL_CODE)
#if defined(GPL_CODE_T2200H) || defined(GPL_CODE)
 #define FACTORY_HOLD_TIME    4
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
 #define FACTORY_BLINK_TIME   3000        /*3000ms*/
#endif
#else
 #define FACTORY_HOLD_TIME    10
#endif
#endif

#if defined(GPL_CODE)
#define RESET_POLL_TIME		1
static int holdTime = 0;
static struct timer_list resetBtnTimer, *pTimer = NULL;
#endif
#if defined(GPL_CODE_FACTORY_TEST)
static int g_RMA_status = 0;
#endif

#if defined(GPL_CODE) || defined(GPL_CODE)
static unsigned short rirq = BP_NOT_DEFINED;
#endif

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
static void __init kerSysInitWatchdogCBList( void );
static int proc_get_watchdog(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_set_watchdog(struct file *f, const char *buf, unsigned long cnt, void *data);
#endif /* defined(CONFIG_BCM_WATCHDOG_TIMER) */
static void start_watchdog(unsigned int timer, unsigned int reset);

// macAddrMutex is used by kerSysGetMacAddress and kerSysReleaseMacAddress
// to protect access to g_pMacInfo
static DEFINE_MUTEX(macAddrMutex);
static PMAC_INFO g_pMacInfo = NULL;
static PGPON_INFO g_pGponInfo = NULL;
static unsigned long g_ulSdramSize;
static int g_ledInitialized = 0;
static wait_queue_head_t g_board_wait_queue;
static CB_DGASP_LIST *g_cb_dgasp_list_head = NULL;
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
static CB_WDOG_LIST *g_cb_wdog_list_head = NULL;
#endif

#define MAX_PAYLOAD_LEN 64
static struct sock *g_monitor_nl_sk;
static int g_monitor_nl_pid = 0 ;
static void kerSysInitMonitorSocket( void );
static void kerSysCleanupMonitorSocket( void );


#if defined(GPL_CODE)
static int  AEI_get_flash_mafId(void);
#endif
#if defined(GPL_CODE)
AEI_WAN_TYPE aeiWanType = AEI_WAN_NONE;
#endif


static int registerBtns(void);

#if defined(CONFIG_BCM_6802_MoCA)
void board_mocaInit(int mocaChipNum);
#endif

static kerSysMacAddressNotifyHook_t kerSysMacAddressNotifyHook = NULL;

/* restore default work structure */
static struct work_struct restoreDefaultWork;
#if defined(GPL_CODE)
/* reboot work structure */
static struct work_struct rebootWork;
#endif

static struct file_operations board_fops =
{
    open:       board_open,
#if defined(HAVE_UNLOCKED_IOCTL)
    unlocked_ioctl: board_unlocked_ioctl,
#else
    ioctl:      board_ioctl,
#endif
    poll:       board_poll,
    read:       board_read,
    release:    board_release,
};

uint32 board_major = 0;
static unsigned short sesBtn_irq = BP_NOT_DEFINED;
static unsigned short sesBtn_gpio = BP_NOT_DEFINED;
static unsigned short sesBtn_polling = 0;
static struct timer_list sesBtn_timer;
static atomic_t sesBtn_active;
static atomic_t sesBtn_forced;
static unsigned short resetBtn_gpio = BP_NOT_DEFINED;
static unsigned short resetBtn2_gpio = BP_NOT_DEFINED;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define NUM_EXT_INT    (INTERRUPT_ID_EXTERNAL_5-INTERRUPT_ID_EXTERNAL_0+1)
#elif defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848) 
#define NUM_EXT_INT    (INTERRUPT_ID_EXTERNAL_7-INTERRUPT_ID_EXTERNAL_0+1)
#else
#define NUM_EXT_INT    (INTERRUPT_ID_EXTERNAL_3-INTERRUPT_ID_EXTERNAL_0+1)
#endif
static unsigned int extIntrInfo[NUM_EXT_INT];

typedef struct
{
    int dev;
    MocaHostIntrCallback mocaCallback;
    void * userArg;
    int irq;
    int intrGpio;
    atomic_t disableCount;
} MOCA_INTR_ARG, *PMOCA_INTR_ARG;

static DEFINE_SPINLOCK(mocaint_spinlock);
static MOCA_INTR_ARG mocaIntrArg[BP_MOCA_MAX_NUM];
static BP_MOCA_INFO mocaInfo[BP_MOCA_MAX_NUM];
static int mocaChipNum = BP_MOCA_MAX_NUM;
static int restore_in_progress = 0;

#if defined (WIRELESS)
static unsigned short sesLed_gpio = BP_NOT_DEFINED;
#endif

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
static DEFINE_SPINLOCK(watchdog_spinlock);
volatile static struct watchdog_struct {
    unsigned int enabled;       // enable watchdog
    unsigned int timer;         // unit is ns
    unsigned int suspend;       // watchdog function is suspended
    unsigned int userMode;      // enable user mode watchdog
    unsigned int userThreshold; // user mode watchdog threshold to reset cpe
    unsigned int userTimeout;   // user mode timeout
} watchdog_data = {0, 5000000, 0, 0, 8, 0};
/* watchdog restart work */
static struct work_struct watchdogRestartWork;
static int watchdog_restart_in_progress = 0;
#endif

#if defined(MODULE)
int init_module(void)
{
    return( brcm_board_init() );
}

void cleanup_module(void)
{
    if (MOD_IN_USE)
        printk("brcm flash: cleanup_module failed because module is in use\n");
    else
        brcm_board_cleanup();
}
#endif //MODULE


#if defined(GPL_CODE)
#include <linux/syscalls.h>
#define BCM_SYSLOG_MAX_LINE_SIZE 256
static int AEI_SaveSyslogOnReboot()
{
    int ret = 0;
    long cfs = 0;
    cfs = sys_open("/tmp/clearonreboot", O_RDONLY, 0);
    if (cfs < 0)
    {
        FLASH_ADDR_INFO fInfo;
        kerSysFlashAddrInfoGet(&fInfo);

#if !defined(GPL_CODE_CONFIG_JFFS)
        if (fInfo.flash_syslog_length >0)
#endif
        {
             struct file *fp;
             int readlen=0;
             char *savebuf,*tempbuf;
             fp = filp_open("/tmp/syslogbak", O_RDONLY, 0);

             if(!IS_ERR(fp) && fp->f_op && fp->f_op->read && fp->f_op->llseek)
             {
#if defined(GPL_CODE_CONFIG_JFFS)
                 int flen;
                 fp->f_op->llseek(fp,0, SEEK_END);
                 flen = fp->f_pos;
                 fp->f_op->llseek(fp,0, SEEK_SET);
                 tempbuf = (char*)kmalloc(flen, GFP_ATOMIC);
                 memset(tempbuf,0,flen);
                 savebuf = (char*)kmalloc(flen+16, GFP_ATOMIC);
                 memset(savebuf,0,flen+16);
#else
                 tempbuf = (char*)kmalloc(fInfo.flash_syslog_length, GFP_ATOMIC);
                 memset(tempbuf,0,fInfo.flash_syslog_length);
                 savebuf = (char*)kmalloc(fInfo.flash_syslog_length, GFP_ATOMIC);
                 memset(savebuf,0,fInfo.flash_syslog_length);
#endif
                 mm_segment_t fs = get_fs();
                 set_fs(get_ds());
#if defined(GPL_CODE_CONFIG_JFFS)
                 readlen = fp->f_op->read(fp, (void *) tempbuf,flen,&fp->f_pos);
#else
                 fp->f_op->llseek(fp, -(fInfo.flash_syslog_length-16), SEEK_END);
                 readlen = fp->f_op->read(fp, (void *) tempbuf, fInfo.flash_syslog_length-16,&fp->f_pos);
#endif
                 if(readlen >0)
                 {
                     int savebuflen=0;
                     int i = 0;
                     char line[BCM_SYSLOG_MAX_LINE_SIZE];
                     char *pblank = NULL;
                     int tzlen = 0;
                     char *linestart,*lineend;
                     char *savebuf_ptr;

                     savebuf_ptr = savebuf+12;
                     linestart = tempbuf;
                     lineend = strchr(linestart,'\n');
                     while(lineend){
                         memset(line,0,BCM_SYSLOG_MAX_LINE_SIZE);
                         if((lineend - linestart)<= BCM_SYSLOG_MAX_LINE_SIZE)
                             strncpy(line,linestart,lineend - linestart +1);
                         else
                             strncpy(line,linestart,BCM_SYSLOG_MAX_LINE_SIZE );

                         linestart = lineend+1;
                         lineend = strchr(linestart,'\n');

                         if((pblank = strchr(line, ')')) != NULL&&(line[0]=='('))
                             tzlen = pblank - line + 1;
                         else
                             continue;

                         if (strlen(line) < tzlen+25 || line[tzlen-1] != ')' || line[tzlen+4] != ' ' ||
                               line[tzlen+8] != '-' || line[tzlen+12] != ' ' || line[tzlen+15] != ' ' ||
                               line[tzlen+24] != ' ')
                             continue;
                         strcat(savebuf_ptr,line);
                         savebuflen += strlen(line);
               //          printk("%d####%s",strlen(line),line);
                    }
                    set_fs(fs);
                    if(savebuflen>0)
                    {
                        char header[16];
                        snprintf(header,sizeof(header),"SYSLOG%06d",savebuflen+16);
                        memcpy(savebuf,header,12);
                        ret = kerSysSyslogSet(savebuf, savebuflen+16,0);
                    }
                    kfree(savebuf);
                }
                filp_close(fp, NULL);
                kfree(tempbuf);
            }
        }
    }
    return ret;
}
#endif


static int map_external_irq (int irq)
{
    int map_irq;
    irq &= ~BP_EXT_INTR_FLAGS_MASK;

    switch (irq) {
    case BP_EXT_INTR_0   :
        map_irq = INTERRUPT_ID_EXTERNAL_0;
        break ;
    case BP_EXT_INTR_1   :
        map_irq = INTERRUPT_ID_EXTERNAL_1;
        break ;
    case BP_EXT_INTR_2   :
        map_irq = INTERRUPT_ID_EXTERNAL_2;
        break ;
    case BP_EXT_INTR_3   :
        map_irq = INTERRUPT_ID_EXTERNAL_3;
        break ;
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
    case BP_EXT_INTR_4   :
        map_irq = INTERRUPT_ID_EXTERNAL_4;
        break ;
    case BP_EXT_INTR_5   :
        map_irq = INTERRUPT_ID_EXTERNAL_5;
        break ;
#endif
#if defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
    case BP_EXT_INTR_6   :
        map_irq = INTERRUPT_ID_EXTERNAL_6;
        break ;
    case BP_EXT_INTR_7   :
        map_irq = INTERRUPT_ID_EXTERNAL_7;
        break ;
#endif
    default           :
        printk ("Invalid External Interrupt definition (%08x)\n", irq) ;
        map_irq = 0 ;
        break ;
    }

    return (map_irq) ;
}

static int set_ext_irq_info(unsigned short ext_irq)
{
	int irq_idx, rc = 0;

	irq_idx = (ext_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;

	if( extIntrInfo[irq_idx] == (unsigned int)(-1) )
		extIntrInfo[irq_idx] = ext_irq;
	else
	{
		/* make sure all the interrupt sharing this irq number has the trigger type and shared */
		if( ext_irq != (unsigned int)extIntrInfo[irq_idx] )
		{
			printk("Invalid ext intr type for BP_EXT_INTR_%d: 0x%x vs 0x%x\r\n", irq_idx, ext_irq, extIntrInfo[irq_idx]);
			extIntrInfo[irq_idx] |= BP_EXT_INTR_CONFLICT_MASK;
			rc = -1;
		}
	}

	return rc;
}

static void init_ext_irq_info(void)
{
	int i, j;
	unsigned short intr;
	void * iter = NULL;
	unsigned short bpBtnIdx, bpGpio, bpExtIrq;

	/* mark each entry invalid */
	for(i=0; i<NUM_EXT_INT; i++)
		extIntrInfo[i] = (unsigned int)(-1);

	/* collect all the external interrupt info from bp */
	if( BpGetResetToDefaultExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);

	if( BpGetResetToDefault2ExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);

	if( BpGetWirelessSesExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);

#if defined CONFIG_BCM963138 || defined CONFIG_BCM963148 
	if( BpGetNfcExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);
#endif

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
	if( BpGetPmdAlarmExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);
#endif

    if( BpGetWifiOnOffExtIntr(&intr) == BP_SUCCESS )
        set_ext_irq_info(intr);

	while(BpGetButtonInfo(&iter, &bpBtnIdx, &bpGpio, &bpExtIrq, NULL, NULL, NULL) == BP_SUCCESS) {
		set_ext_irq_info(bpExtIrq);
        }

	for( i = 0; i < mocaChipNum; i++ )
	{
		for( j = 0; j < BP_MOCA_MAX_INTR_NUM; j++ )
		{
			if( mocaInfo[i].intr[j] != BP_NOT_DEFINED )
			{
#if defined(CONFIG_BCM96838)
				unsigned short irqIdx = map_external_irq(mocaInfo[i].intr[j]) - INTERRUPT_ID_EXTERNAL_0;
				unsigned short gpio = mocaInfo[i].intrGpio[j];
				gpio &= BP_GPIO_NUM_MASK;
				PERF->ext_irq_muxsel0 |= ( (gpio&EXT_IRQ_MASK_LOW) << (irqIdx*EXT_IRQ_OFF_LOW) );
				DBGPERF->Dbg_extirqmuxsel0_1 |= ( ((gpio&EXT_IRQ_MASK_HIGH)>>EXT_IRQ_OFF_LOW) << (irqIdx*EXT_IRQ_OFF_HIGH) );
#endif
				set_ext_irq_info(mocaInfo[i].intr[j]);
			}
		}
	}
	return;
}

PBP_MOCA_INFO boardGetMocaInfo(int dev)
{
	if( dev >= mocaChipNum)
		return NULL;
	else
		return &mocaInfo[dev];
}

#if defined(CONFIG_BCM960333)
static void mapBcm960333GpioToIntr( unsigned int gpio, unsigned long extIrq )
{
    unsigned int extIrqIdx = map_external_irq(extIrq) - INTERRUPT_ID_EXTERNAL_0;
    volatile uint32 * pMuxReg = &(GPIO->GPIOMuxCtrl_0) + gpio/4;
    int gpioShift = (gpio % 4) * 8;
    uint32 gpioMask = 0x7f << gpioShift;

    BcmHalExternalIrqMask(extIrqIdx);
    *pMuxReg = (*pMuxReg & (~gpioMask)) | (extIrqIdx << gpioShift);
    GPIO->GPIOFuncMode |= 1 << gpio;
}
#endif

static int kerSysIsBatteryEnabled(void)
{
#if defined(CONFIG_BCM_BMU)
    unsigned short bmuen;

    if (BpGetBatteryEnable(&bmuen) == BP_SUCCESS) {
        return (bmuen);
    }
#endif
    return 0;
}

static void __init kerSysInitBatteryManagementUnit(void)
{
#if defined(CONFIG_BCM_BMU)
    if (kerSysIsBatteryEnabled()) {
        pmc_apm_power_up();
#if defined(CONFIG_BCM963148)
        // APM_ANALOG_BG_BOOST and APM_LDO_VREGCNTL_7 default to 0 in 63148 and need to be set
        APM_PUB->reg_apm_analog_bg |= APM_ANALOG_BG_BOOST;
        APM_PUB->reg_codec_config_4 |= APM_LDO_VREGCNTL_7;
#endif
    }
#endif
}

/* A global variable used by Power Management and other features to determine if Voice is idle or not */
volatile int isVoiceIdle = 1;
EXPORT_SYMBOL(isVoiceIdle);

int ext_irq_connect(int irq, unsigned int param, FN_HANDLER isr)
{
    int rc = 0;

    irq = map_external_irq(irq);
    rc = BcmHalMapInterrupt(isr, param, irq);
#if !defined(CONFIG_ARM)
    if (!rc)
        BcmHalInterruptEnable(irq);
#endif
    return rc;
}
EXPORT_SYMBOL(ext_irq_connect);

void ext_irq_enable(int irq, int enable)
{
    irq = map_external_irq(irq);
#if !defined(CONFIG_ARM)
    if (enable)
        BcmHalInterruptEnable(irq);
    else
        BcmHalInterruptDisable(irq);
#endif
}
EXPORT_SYMBOL(ext_irq_enable);

#ifdef  AEI_DETECT_BOARD_ID
#define GPIO_BID_DL_L   139
#define GPIO_BID_SD_CLK 141
#define GPIO_BID_DATA   140

int AEI_get_boardid(void);
extern struct proc_dir_entry proc_root;
static int boardid_proc_read(char *buf, char **start, off_t off, int cnt, int *eof, void *data)
{
    int len=0;

    len = sprintf(buf, "%02x", AEI_get_boardid());

    return len;
}

int AEI_kerSysGetGpioValue(unsigned bpGpio)
{
    if(kerSysGetGpioValue(bpGpio)){
        return 1;
    }else{
        return 0;
    }
}

int AEI_get_boardid(void)
{
    int i;
    static int boardid=0;

    if(boardid>0) return boardid;

    /* set bid_data gpio as input*/
    kerSysSetGpioDirInput(GPIO_BID_DATA);
    /* set shld to low level */
    kerSysSetGpioState(GPIO_BID_DL_L, kGpioInactive);
    /* set shld to high level */
    kerSysSetGpioState(GPIO_BID_DL_L, kGpioActive);

    for(i=0;i < 8;i++){
        /* set clock to low level */
        kerSysSetGpioState(GPIO_BID_SD_CLK, kGpioInactive);
        boardid = boardid << 1 | AEI_kerSysGetGpioValue(GPIO_BID_DATA);
        /* set clock to high level */
        kerSysSetGpioState(GPIO_BID_SD_CLK, kGpioActive);
    }
    kerSysSetGpioState(GPIO_BID_DL_L, kGpioInactive);

    return boardid;
}
EXPORT_SYMBOL(AEI_get_boardid);
#endif
static int __init brcm_board_init( void )
{
	int ret;
#if defined(GPL_CODE)
    char board_id[32]={0};
#endif
#ifdef CONFIG_BCM968500
	extern unsigned long bl_lilac_get_phys_mem_size(void);

    ret = register_chrdev(BOARD_DRV_MAJOR, "brcmboard", &board_fops );
    if (ret < 0)
        printk( "brcm_board_init(major %d): fail to register device.\n",BOARD_DRV_MAJOR);
    else
    {
        printk("brcmboard: brcm_board_init entry\n");
        board_major = BOARD_DRV_MAJOR;

		g_ulSdramSize = bl_lilac_get_phys_mem_size();

		set_mac_info();
#if defined(CONFIG_BCM_GPON_STACK) || defined(CONFIG_BCM_GPON_STACK_MODULE)
		set_gpon_info();
#endif

		init_waitqueue_head(&g_board_wait_queue);
#if defined (WIRELESS)
		kerSysScreenPciDevices();
#endif

		ses_board_init();
		
		kerSysInitMonitorSocket();
		kerSysInitDyingGaspHandler();
		boardLedInit();
		g_ledInitialized = 1;
    }

    add_proc_files();

#else // CONFIG_BCM968500
    unsigned short rstToDflt_irq;
    unsigned short rstToDflt2_irq;
    int ext_irq_idx;
    int ext_irq2_idx;
    bcmLogSpiCallbacks_t loggingCallbacks;

    ret = register_chrdev(BOARD_DRV_MAJOR, "brcmboard", &board_fops );
    if (ret < 0)
        printk( "brcm_board_init(major %d): fail to register device.\n",BOARD_DRV_MAJOR);
    else
    {
        printk("brcmboard: brcm_board_init entry\n");
        board_major = BOARD_DRV_MAJOR;

        g_ulSdramSize = getMemorySize();
        set_mac_info();
        set_gpon_info();

        BpGetMocaInfo(mocaInfo, &mocaChipNum);

        init_ext_irq_info();

        init_waitqueue_head(&g_board_wait_queue);
#if defined (WIRELESS)
        kerSysScreenPciDevices();
        kerSetWirelessPD(WLAN_ON);
#endif
        ses_board_init();
#if defined CONFIG_BCM963138 || defined CONFIG_BCM963148
        nfc_board_init();
#endif

        kerSysInitMonitorSocket();
        kerSysInitDyingGaspHandler();
#if defined(CONFIG_BCM96318)
        kerSysInit6318Reset();
#endif
        kerSysInitBatteryManagementUnit();
       
#if defined(CONFIG_BCM963381) && !IS_ENABLED(CONFIG_BCM_ADSL)
        /* Enable  dsl mips to workaround WD reset issue when dsl is not built */
        /* DSL power up is done in kerSysInitDyingGaspHandler */
        pmc_dsl_mips_enable(1);
#endif
        boardLedInit();
        g_ledInitialized = 1;

        if( BpGetResetToDefaultExtIntr(&rstToDflt_irq) == BP_SUCCESS )
        {
            ext_irq_idx = (rstToDflt_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
#if defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
            kerSysInitPinmuxInterface(BP_PINMUX_FNTYPE_IRQ | ext_irq_idx);
#endif


#if defined(CONFIG_BCM960333)
            if (BpGetResetToDefaultExtIntrGpio(&resetBtn_gpio) == BP_SUCCESS) {
                resetBtn_gpio &= BP_GPIO_NUM_MASK;
                if ( ext_irq_idx  != BP_NOT_DEFINED && resetBtn_gpio != BP_NOT_DEFINED ) {
                    mapBcm960333GpioToIntr( resetBtn_gpio, rstToDflt_irq );
                }
            }
#endif
            
            if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
            {
                static int dev = -1;
                int hookisr = 1;

                if (IsExtIntrShared(rstToDflt_irq))
                {
                    /* get the gpio and make it input dir */
                    if( (resetBtn_gpio != BP_NOT_DEFINED) || (BpGetResetToDefaultExtIntrGpio(&resetBtn_gpio) == BP_SUCCESS) )
                    {
                    	resetBtn_gpio &= BP_GPIO_NUM_MASK;
                        printk("brcm_board_init: Reset config Interrupt gpio is %d\n", resetBtn_gpio);
                        kerSysSetGpioDirInput(resetBtn_gpio); // Set gpio for input.
                        dev = resetBtn_gpio;
                    }
                    else
                    {
                        printk("brcm_board_init: Reset config gpio definition not found \n");
                        hookisr= 0;
                    }
                }
                
                if(hookisr)
                {
                    ext_irq_connect(rstToDflt_irq, (unsigned int)&dev, (FN_HANDLER)reset_isr);
                }
            }
        }

        if( BpGetResetToDefault2ExtIntr(&rstToDflt2_irq) == BP_SUCCESS )
        {
            ext_irq2_idx = (rstToDflt2_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
#if defined(CONFIG_BCM960333)
            if (BpGetResetToDefault2ExtIntrGpio(&resetBtn2_gpio) == BP_SUCCESS) {
                resetBtn2_gpio &= BP_GPIO_NUM_MASK;
                if ( ext_irq2_idx  != BP_NOT_DEFINED && resetBtn2_gpio != BP_NOT_DEFINED ) {
                    mapBcm960333GpioToIntr( resetBtn2_gpio, rstToDflt2_irq );
                }
            }
#endif

            if (!IsExtIntrConflict(extIntrInfo[ext_irq2_idx]))
            {
                static int dev = -1;
                int hookisr = 1;

                if (IsExtIntrShared(rstToDflt2_irq))
                {
                    /* get the gpio and make it input dir */
                    if( (resetBtn2_gpio != BP_NOT_DEFINED) || (BpGetResetToDefault2ExtIntrGpio(&resetBtn2_gpio) == BP_SUCCESS) )
                    {
                        resetBtn2_gpio &= BP_GPIO_NUM_MASK;
                        kerSysSetGpioDirInput(resetBtn2_gpio); // Set gpio for input.
                        dev = resetBtn2_gpio;
                    }
                    else
                    {
                        hookisr= 0;
                    }
                }
                
                if(hookisr)
                {
                    ext_irq_connect(rstToDflt2_irq, (unsigned int)&dev, (FN_HANDLER)reset_isr);
                }
            }
            else 
            {
                printk("[%s.%d]: conflict exists %d (%08x)\n", __func__, __LINE__, ext_irq2_idx, extIntrInfo[ext_irq2_idx]);
            }
        }

#if defined(CONFIG_BCM_EXT_TIMER)
        init_hw_timers();
#endif
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
        kerSysInitWatchdogCBList();
#endif

#if defined(CONFIG_BCM_CPLD1)
        // Reserve SPI bus to control external CPLD for Standby Timer
        BcmCpld1Initialize();
#endif
    }

    registerBtns();

    add_proc_files();

#if   defined(CONFIG_BCM_6802_MoCA)
#if defined(GPL_CODE_MULT_BOARD_ID_IN_ONE_FIRMWARE)
    kerSysNvRamGetBoardId(board_id);
    if (!strcmp(board_id, "T3200B") || !strcmp(board_id, "C3000") ||
        !strcmp(board_id, "T3250") || !strcmp(board_id, "T3250V") ||
        !strcmp(board_id, "T3255") || !strcmp(board_id, "T3255V") 
        ){
        printk("%s no support Moca\n", board_id);
    } else
#endif
    {
    board_mocaInit(mocaChipNum);
    loggingCallbacks.kerSysSlaveRead   = kerSysBcmSpiSlaveRead;
    loggingCallbacks.kerSysSlaveWrite  = kerSysBcmSpiSlaveWrite;
    loggingCallbacks.bpGet6829PortInfo = NULL;
    }
#endif
    loggingCallbacks.reserveSlave      = BcmSpiReserveSlave;
    loggingCallbacks.syncTrans         = BcmSpiSyncTrans;
    bcmLog_registerSpiCallbacks(loggingCallbacks);

#endif //#ifdef CONFIG_BCM968500
#if defined(GPL_CODE)
    kerSysNvRamGetBoardId(board_id);

    if (strstr(board_id, "C2000") != NULL)
        aeiBoardId = AEI_BOARD_C2000A;
    else if (strstr(board_id, "C1900A") != NULL)
        aeiBoardId = AEI_BOARD_C2000A;
    else if (strstr(board_id, "C3000") != NULL)
        aeiBoardId = AEI_BOARD_C3000A;
    else if (strstr(board_id, "V2000") != NULL)
        aeiBoardId = AEI_BOARD_V2000H;
    else if (strstr(board_id, "FV2200") != NULL)
        aeiBoardId = AEI_BOARD_FV2200;
    else if (strstr(board_id, "FV2210") != NULL)
        aeiBoardId = AEI_BOARD_FV2210;
    else if (strstr(board_id, "FV2250") != NULL)
        aeiBoardId = AEI_BOARD_FV2250;
    else if (strstr(board_id, "V1000H") != NULL)
        aeiBoardId = AEI_BOARD_V1000H;
    else if (strstr(board_id, "VB784WG") != NULL)
        aeiBoardId = AEI_BOARD_V1000H;
    else if (strstr(board_id, "V2200H") != NULL)
        aeiBoardId = AEI_BOARD_V2200H;
    else if (strstr(board_id, "T2200H") != NULL)
        aeiBoardId = AEI_BOARD_T2200H;
    else if (strstr(board_id, "T3200") != NULL)
        aeiBoardId = AEI_BOARD_T3200;
    else if (strstr(board_id, "T3250") != NULL)
        aeiBoardId = AEI_BOARD_T3200;
    else if (strstr(board_id, "T3255") != NULL)
        aeiBoardId = AEI_BOARD_T3200;
    else if (strstr(board_id, "T1200H") != NULL)
        aeiBoardId = AEI_BOARD_T1200H;
    else if (strstr(board_id, "F3500") != NULL)
        aeiBoardId = AEI_BOARD_F3500;
#endif
#if defined(GPL_CODE_CHIP_D0_CHECK)
    AEI_setExpSerialNumber();
#endif
    return ret;
}

static void __init set_mac_info( void )
{
    NVRAM_DATA *pNvramData;
    unsigned long ulNumMacAddrs;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("set_mac_info: could not read nvram data\n");
        return;
    }

    ulNumMacAddrs = pNvramData->ulNumMacAddrs;

#if defined(GPL_CODE)
    /* If MAC OUI is 00-26-88, then re-write to 10:9F:A9 not 40-8b-07 */
    if (pNvramData->ucaBaseMacAddr[0]==0x0 && pNvramData->ucaBaseMacAddr[1]==0x26 && pNvramData->ucaBaseMacAddr[2]==0x88)
    {
        int spot = sizeof(pNvramData->chUnused);
        pNvramData->ucaBaseMacAddr[0]=0x10;
        pNvramData->ucaBaseMacAddr[1]=0x9f;
        pNvramData->ucaBaseMacAddr[2]=0xa9;
        /* need to find new storage for CL images to mark if MAC address rewritten
           as new SDK does not preserve chReserved so use last 2 bytes in chUnused as that is constant place
         */
        //pNvramData->chReserved[0]='f';
        //pNvramData->chReserved[1]='u';
        if (spot > 0)
           pNvramData->chUnused[spot-1]='u';
        if (spot > 1)
           pNvramData->chUnused[spot-2]='f';
        writeNvramDataCrcLocked(pNvramData);
    }
#endif

    if( ulNumMacAddrs > 0 && ulNumMacAddrs <= NVRAM_MAC_COUNT_MAX )
    {
        unsigned long ulMacInfoSize =
            sizeof(MAC_INFO) + ((sizeof(MAC_ADDR_INFO)) * (ulNumMacAddrs-1));

        g_pMacInfo = (PMAC_INFO) kmalloc( ulMacInfoSize, GFP_KERNEL );

        if( g_pMacInfo )
        {
            memset( g_pMacInfo, 0x00, ulMacInfoSize );
            g_pMacInfo->ulNumMacAddrs = pNvramData->ulNumMacAddrs;
            memcpy( g_pMacInfo->ucaBaseMacAddr, pNvramData->ucaBaseMacAddr,
                NVRAM_MAC_ADDRESS_LEN );
        }
        else
            printk("ERROR - Could not allocate memory for MAC data\n");
    }
    else
        printk("ERROR - Invalid number of MAC addresses (%ld) is configured.\n",
        ulNumMacAddrs);
    kfree(pNvramData);
}

static int gponParamsAreErased(NVRAM_DATA *pNvramData)
{
    int i;
    int erased = 1;

    for(i=0; i<NVRAM_GPON_SERIAL_NUMBER_LEN-1; ++i) {
        if((pNvramData->gponSerialNumber[i] != (char) 0xFF) &&
            (pNvramData->gponSerialNumber[i] != (char) 0x00)) {
                erased = 0;
                break;
        }
    }

    if(!erased) {
        for(i=0; i<NVRAM_GPON_PASSWORD_LEN-1; ++i) {
            if((pNvramData->gponPassword[i] != (char) 0xFF) &&
                (pNvramData->gponPassword[i] != (char) 0x00)) {
                    erased = 0;
                    break;
            }
        }
    }

    return erased;
}

static void __init set_gpon_info( void )
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("set_gpon_info: could not read nvram data\n");
        return;
    }

    g_pGponInfo = (PGPON_INFO) kmalloc( sizeof(GPON_INFO), GFP_KERNEL );

    if( g_pGponInfo )
    {
        if ((pNvramData->ulVersion < NVRAM_FULL_LEN_VERSION_NUMBER) ||
            gponParamsAreErased(pNvramData))
        {
            strcpy( g_pGponInfo->gponSerialNumber, DEFAULT_GPON_SN );
            strcpy( g_pGponInfo->gponPassword, DEFAULT_GPON_PW );
        }
        else
        {
            strncpy( g_pGponInfo->gponSerialNumber, pNvramData->gponSerialNumber,
                NVRAM_GPON_SERIAL_NUMBER_LEN );
            g_pGponInfo->gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN-1]='\0';
            strncpy( g_pGponInfo->gponPassword, pNvramData->gponPassword,
                NVRAM_GPON_PASSWORD_LEN );
            g_pGponInfo->gponPassword[NVRAM_GPON_PASSWORD_LEN-1]='\0';
        }
    }
    else
    {
        printk("ERROR - Could not allocate memory for GPON data\n");
    }
    kfree(pNvramData);
}

void __exit brcm_board_cleanup( void )
{
    printk("brcm_board_cleanup()\n");
    del_proc_files();

    if (board_major != -1)
    {
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
        ses_board_deinit();
#endif
        kerSysDeinitDyingGaspHandler();
        kerSysCleanupMonitorSocket();
        unregister_chrdev(board_major, "board_ioctl");

    }
}

static BOARD_IOC* borad_ioc_alloc(void)
{
    BOARD_IOC *board_ioc =NULL;
    board_ioc = (BOARD_IOC*) kmalloc( sizeof(BOARD_IOC) , GFP_KERNEL );
    if(board_ioc)
    {
        memset(board_ioc, 0, sizeof(BOARD_IOC));
    }
    return board_ioc;
}

static void borad_ioc_free(BOARD_IOC* board_ioc)
{
    if(board_ioc)
    {
        kfree(board_ioc);
    }
}


static int board_open( struct inode *inode, struct file *filp )
{
    filp->private_data = borad_ioc_alloc();

    if (filp->private_data == NULL)
        return -ENOMEM;

    return( 0 );
}

static int board_release(struct inode *inode, struct file *filp)
{
    BOARD_IOC *board_ioc = filp->private_data;

    wait_event_interruptible(g_board_wait_queue, 1);
    borad_ioc_free(board_ioc);

    return( 0 );
}


static unsigned int board_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
#if defined (WIRELESS)
    BOARD_IOC *board_ioc = filp->private_data;
#endif

    poll_wait(filp, &g_board_wait_queue, wait);
#if defined (WIRELESS)
    if(board_ioc->eventmask & SES_EVENTS){
        mask |= sesBtn_poll(filp, wait);
    }
#endif

    return mask;
}

static ssize_t board_read(struct file *filp,  char __user *buffer, size_t count, loff_t *ppos)
{
#if defined (WIRELESS)
    BOARD_IOC *board_ioc = filp->private_data;
    if(board_ioc->eventmask & SES_EVENTS){
        return sesBtn_read(filp, buffer, count, ppos);
    }
#endif
    return 0;
}

/***************************************************************************
// Function Name: getCrc32
// Description  : caculate the CRC 32 of the given data.
// Parameters   : pdata - array of data.
//                size - number of input data bytes.
//                crc - either CRC32_INIT_VALUE or previous return value.
// Returns      : crc.
****************************************************************************/
static UINT32 getCrc32(byte *pdata, UINT32 size, UINT32 crc)
{
    while (size-- > 0)
        crc = (crc >> 8) ^ Crc32_table[(crc ^ *pdata++) & 0xff];

    return crc;
}

/** calculate the CRC for the nvram data block and write it to flash.
 * Must be called with flashImageMutex held.
 */
static void writeNvramDataCrcLocked(PNVRAM_DATA pNvramData)
{
    UINT32 crc = CRC32_INIT_VALUE;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    pNvramData->ulCheckSum = 0;
    crc = getCrc32((char *)pNvramData, sizeof(NVRAM_DATA), crc);
    pNvramData->ulCheckSum = crc;
    kerSysNvRamSet((char *)pNvramData, sizeof(NVRAM_DATA), 0);
}


/** read the nvramData struct from the in-memory copy of nvram.
 * The caller is not required to have flashImageMutex when calling this
 * function.  However, if the caller is doing a read-modify-write of
 * the nvram data, then the caller must hold flashImageMutex.  This function
 * does not know what the caller is going to do with this data, so it
 * cannot assert flashImageMutex held or not when this function is called.
 *
 * @return pointer to NVRAM_DATA buffer which the caller must free
 *         or NULL if there was an error
 */
static PNVRAM_DATA readNvramData(void)
{
    UINT32 crc = CRC32_INIT_VALUE, savedCrc;
    NVRAM_DATA *pNvramData;

    // use GFP_ATOMIC here because caller might have flashImageMutex held
    if (NULL == (pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_ATOMIC)))
    {
        printk("readNvramData: could not allocate memory\n");
        return NULL;
    }

    kerSysNvRamGet((char *)pNvramData, sizeof(NVRAM_DATA), 0);
    savedCrc = pNvramData->ulCheckSum;
    pNvramData->ulCheckSum = 0;
    crc = getCrc32((char *)pNvramData, sizeof(NVRAM_DATA), crc);
    if (savedCrc != crc)
    {
        // this can happen if we write a new cfe image into flash.
        // The new image will have an invalid nvram section which will
        // get updated to the inMemNvramData.  We detect it here and
        // commonImageWrite will restore previous copy of nvram data.
        kfree(pNvramData);
        pNvramData = NULL;
    }

    return pNvramData;
}
#if defined(GPL_CODE)
static int AEI_readNvramData(PNVRAM_DATA pNvramData)
{
    UINT32 crc = CRC32_INIT_VALUE, savedCrc;

    kerSysNvRamGet((char *)pNvramData, sizeof(NVRAM_DATA), 0);
    savedCrc = pNvramData->ulCheckSum;
    pNvramData->ulCheckSum = 0;
    crc = getCrc32((char *)pNvramData, sizeof(NVRAM_DATA), crc);
    if (savedCrc != crc)
        return -1;

    return 0;
}
#endif
#if defined(GPL_CODE_CHIP_D0_CHECK)
static void AEI_setExpSerialNumber()
{
    NVRAM_DATA nvramData;
    AEI_readNvramData(&nvramData);
    memset(CPUSerialNumber, 0 , 32);
    memcpy(CPUSerialNumber, &nvramData.ulSerialNumber[0] , 32 - 1);
}
#endif
//**************************************************************************************
// Utitlities for dump memory, free kernel pages, mips soft reset, etc.
//**************************************************************************************

/***********************************************************************
* Function Name: dumpaddr
* Description  : Display a hex dump of the specified address.
***********************************************************************/
void dumpaddr( unsigned char *pAddr, int nLen )
{
    static char szHexChars[] = "0123456789abcdef";
    char szLine[80];
    char *p = szLine;
    unsigned char ch, *q;
    int i, j;
    unsigned long ul;

    while( nLen > 0 )
    {
        sprintf( szLine, "%8.8lx: ", (unsigned long) pAddr );
        p = szLine + strlen(szLine);

        for(i = 0; i < 16 && nLen > 0; i += sizeof(long), nLen -= sizeof(long))
        {
            ul = *(unsigned long *) &pAddr[i];
            q = (unsigned char *) &ul;
            for( j = 0; j < sizeof(long); j++ )
            {
                *p++ = szHexChars[q[j] >> 4];
                *p++ = szHexChars[q[j] & 0x0f];
                *p++ = ' ';
            }
        }

        for( j = 0; j < 16 - i; j++ )
            *p++ = ' ', *p++ = ' ', *p++ = ' ';

        *p++ = ' ', *p++ = ' ', *p++ = ' ';

        for( j = 0; j < i; j++ )
        {
            ch = pAddr[j];
            *p++ = (ch > ' ' && ch < '~') ? ch : '.';
        }

        *p++ = '\0';
        printk( "%s\r\n", szLine );

        pAddr += i;
    }
    printk( "\r\n" );
} /* dumpaddr */


/* On 6318, the Sleep mode is used to force a reset on PVT Monitor and ASB blocks */
#if defined(CONFIG_BCM96318)
static void __init kerSysInit6318Reset( void )
{
    /* Re-initialize the sleep registers because they are not cleared on reset */
    RTC->RtcSleepRtcEnable &= ~RTC_SLEEP_RTC_ENABLE;
    RTC->RtcSleepRtcEvent &= ~RTC_SLEEP_RTC_EVENT;

    /* A magic word is saved in scratch register to identify unintentional resets */
    /* (Scratch register is not cleared on reset) */
    if (RTC->RtcSleepCpuScratchpad == 0x600DBEEF) {
        /* When Magic word is seen during reboot, there was an unintentional reset */
        printk("Previous reset was unintentional, performing full reset sequence\n");
        kerSysMipsSoftReset();
    }
    RTC->RtcSleepCpuScratchpad = 0x600DBEEF;
}

static void kerSys6318Reset( void )
{
    unsigned short plcGpio;

    /* Use GPIO to control the PLC and wifi chip reset on 6319 PLC board */
    if( BpGetPLCPwrEnGpio(&plcGpio) == BP_SUCCESS )
    {
        kerSysSetGpioState(plcGpio, kGpioInactive);
        /* Delay to ensure WiFi and PLC are powered down */
        udelay(5000);
    }

    /* On 6318, the Sleep mode is used to force a reset on PVT Monitor and ASB blocks */
    /* Configure the sleep mode and duration */
    /* then wait for system to come out of reset when the timer expires */
    PLL_PWR->PllPwrControlIddqCtrl &= ~IDDQ_SLEEP;
    RTC->RtcSleepWakeupMask = RTC_SLEEP_RTC_IRQ;
    RTC->RtcSleepCpuScratchpad = 0x01010101; // We are intentionally starting the reset sequence
    RTC->RtcSleepRtcCountL = 0x00020000; // Approx 5 ms
    RTC->RtcSleepRtcCountH = 0x0;
    RTC->RtcSleepRtcEnable = RTC_SLEEP_RTC_ENABLE;
    RTC->RtcSleepModeEnable = RTC_SLEEP_MODE_ENABLE;
    // while(1); // Spin occurs in calling function.  Do not spin here
}
#endif


/** this function actually does two things, stop other cpu and reset mips.
 * Kept the current name for compatibility reasons.  Image upgrade code
 * needs to call the two steps separately.
 */
void kerSysMipsSoftReset(void)
{
	unsigned long cpu;
#if defined(GPL_CODE) && (defined(CONFIG_BCM963138) /*|| defined(CONFIG_BCM963268)*/)
	set_bootstatus(SW_NORMAL_RESET_STATUS);
#endif
	cpu = smp_processor_id();
	printk(KERN_INFO "kerSysMipsSoftReset: called on cpu %lu\n", cpu);
	// FIXME - Once in many thousands of reboots, this function could 
	// fail to complete a reboot.  Arm the watchdog just in case.
#if !defined(CONFIG_BCM96318)
	start_watchdog(5, 1);
#endif

	stopOtherCpu();
	local_irq_disable();  // ignore interrupts, just execute reset code now
	resetPwrmgmtDdrMips();
}

extern void stop_other_cpu(void);  // in arch/mips/kernel/smp.c

void stopOtherCpu(void)
{
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#if defined(CONFIG_SMP)
    /* in ARM, CPU#0 should be the last one to get shut down, and for
     * both 63138 and 63148, we have dualcore system, so we can hardcode
     * cpu_down() on CPU#1. Also, if this function is handled by the 
     * CPU which is going to be shut down, kernel will transfer the
     * current task to another CPU.  Thus when we return from cpu_down(),
     * the task is still running. */
    cpu_down(1);
#endif
#else
#if defined(CONFIG_SMP)
    stop_other_cpu();
#elif defined(CONFIG_BCM_ENDPOINT_MODULE) && defined(CONFIG_BCM_BCMDSP_MODULE)
    unsigned long cpu = (read_c0_diag3() >> 31) ? 0 : 1;

	// Disable interrupts on the other core and allow it to complete processing 
	// and execute the "wait" instruction
    printk(KERN_INFO "stopOtherCpu: stopping cpu %lu\n", cpu);	
    PERF->IrqControl[cpu].IrqMask = 0;
    mdelay(5);
#endif
#endif /* !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) */
}

void resetPwrmgmtDdrMips(void)
{
#if   defined(CONFIG_BCM96838)
    WDTIMER->WD0DefCount = 0;
    WDTIMER->WD0Ctl = 0xee00;
    WDTIMER->WD0Ctl = 0x00ee;
    WDTIMER->WD1DefCount = 0;
    WDTIMER->WD1Ctl = 0xee00;
    WDTIMER->WD1Ctl = 0x00ee;
    PERF->TimerControl |= SOFT_RESET_0;
#else
#if defined (CONFIG_BCM963268)
    MISC->miscVdslControl &= ~(MISC_VDSL_CONTROL_VDSL_MIPS_RESET | MISC_VDSL_CONTROL_VDSL_MIPS_POR_RESET );
#endif
#if !defined (CONFIG_BCM96838) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM960333) && !defined(CONFIG_BCM963148) && !defined(CONFIG_BCM963381) && !defined(CONFIG_BCM96848)
    // Power Management on Ethernet Ports may have brought down EPHY PLL
    // and soft reset below will lock-up 6362 if the PLL is not up
    // therefore bring it up here to give it time to stabilize
    GPIO->RoboswEphyCtrl &= ~EPHY_PWR_DOWN_DLL;
#endif
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* stop SF2 switch from sending packet to runner, or the DMA might get stuck.
     * Also give it time to complete the ongoing DMA transaction. */
    ETHSW_CORE->imp_port_state &= ~ETHSW_IPS_LINK_PASS;
#endif

    // let UART finish printing
    udelay(100);


#if defined(CONFIG_BCM_CPLD1)
    // Determine if this was a request to enter Standby mode
    // If yes, this call won't return and a hard reset will occur later
    BcmCpld1CheckShutdownMode();
#endif


#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
    TIMER->SoftRst = 1;
#elif defined(CONFIG_BCM96318)
    kerSys6318Reset();
#elif defined(CONFIG_BCM960333)
    /*
     * After a soft-reset, one of the reserved bits of TIMER->SoftRst remains
     * enabled and the next soft-reset won't work unless TIMER->SoftRst is
     * set to 0.
     */
    TIMER->SoftRst = 0;
    TIMER->SoftRst |= SOFT_RESET;
#else
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
    PERF->pll_control |= SOFT_RESET;    // soft reset mips
#endif
#endif
#endif
    for(;;) {} // spin mips and wait soft reset to take effect
}

unsigned long kerSysGetMacAddressType( unsigned char *ifName )
{
    unsigned long macAddressType = MAC_ADDRESS_ANY;

    if(strstr(ifName, IF_NAME_ETH))
    {
        macAddressType = MAC_ADDRESS_ETH;
    }
#if defined (GPL_CODE_WAN_ETH)
    else if (strstr(ifName, IF_NAME_EWAN))
    {
        macAddressType = MAC_ADDRESS_ETH;
    }
#endif

    else if(strstr(ifName, IF_NAME_USB))
    {
        macAddressType = MAC_ADDRESS_USB;
    }
    else if(strstr(ifName, IF_NAME_WLAN))
    {
        macAddressType = MAC_ADDRESS_WLAN;
    }
    else if(strstr(ifName, IF_NAME_MOCA))
    {
        macAddressType = MAC_ADDRESS_MOCA;
    }
    else if(strstr(ifName, IF_NAME_ATM))
    {
        macAddressType = MAC_ADDRESS_ATM;
    }
    else if(strstr(ifName, IF_NAME_PTM))
    {
#if defined(GPL_CODE_STATIC_MAC_ADDR)
        macAddressType = 0x12ffffff;
#else
        macAddressType = MAC_ADDRESS_PTM;
#endif
    }
    else if(strstr(ifName, IF_NAME_GPON) || strstr(ifName, IF_NAME_VEIP))
    {
        macAddressType = MAC_ADDRESS_GPON;
    }
    else if(strstr(ifName, IF_NAME_EPON))
    {
        macAddressType = MAC_ADDRESS_EPON;
    }

    return macAddressType;
}

static inline void kerSysMacAddressNotify(unsigned char *pucaMacAddr, MAC_ADDRESS_OPERATION op)
{
    if(kerSysMacAddressNotifyHook)
    {
        kerSysMacAddressNotifyHook(pucaMacAddr, op);
    }
}

int kerSysMacAddressNotifyBind(kerSysMacAddressNotifyHook_t hook)
{
    int nRet = 0;

    if(hook && kerSysMacAddressNotifyHook)
    {
        printk("ERROR: kerSysMacAddressNotifyHook already registered! <0x%08lX>\n",
               (unsigned long)kerSysMacAddressNotifyHook);
        nRet = -EINVAL;
    }
    else
    {
        kerSysMacAddressNotifyHook = hook;
    }

    return nRet;
}

#if defined (WIRELESS)
void kerSysSesEventTrigger( int forced )
{
   if (forced) {
      atomic_set (&sesBtn_forced, 1);
   }
   wake_up_interruptible(&g_board_wait_queue);
}
#endif


static void getNthMacAddr( unsigned char *pucaMacAddr, unsigned long n)
{
    unsigned long macsequence = 0;
    /* Work with only least significant three bytes of base MAC address */
    macsequence = (pucaMacAddr[3] << 16) | (pucaMacAddr[4] << 8) | pucaMacAddr[5];
    macsequence = (macsequence + n) & 0xffffff;
    pucaMacAddr[3] = (macsequence >> 16) & 0xff;
    pucaMacAddr[4] = (macsequence >> 8) & 0xff;
    pucaMacAddr[5] = (macsequence ) & 0xff;

}
static unsigned long getIdxForNthMacAddr( const unsigned char *pucaBaseMacAddr, unsigned char *pucaMacAddr)
{
    unsigned long macSequence = 0;
    unsigned long baseMacSequence = 0;
    
    macSequence = (pucaMacAddr[3] << 16) | (pucaMacAddr[4] << 8) | pucaMacAddr[5];
    baseMacSequence = (pucaBaseMacAddr[3] << 16) | (pucaBaseMacAddr[4] << 8) | pucaBaseMacAddr[5];

    return macSequence - baseMacSequence;
}

#if defined(GPL_CODE)
#define AEI_INVALID_ASSIGN_ID -1
#endif

/* Allocates requested number of consecutive MAC addresses */
int kerSysGetMacAddresses( unsigned char *pucaMacAddr, unsigned int num_addresses, unsigned long ulId )
{
    int nRet = -EADDRNOTAVAIL;
    PMAC_ADDR_INFO pMai = NULL;
    PMAC_ADDR_INFO pMaiFreeId = NULL, pMaiFreeIdTemp;
    unsigned long i = 0, j = 0, ulIdxId = 0;

#if defined(GPL_CODE)
    unsigned long baseMacAddr = 0;
    const unsigned long constMacAddrIncIndex = 3;
#endif

#if defined(GPL_CODE)
    UINT32 valueType = 0;
    UINT32 valueNum = 0;
    UINT32 valueId = 0;
    int  assignIndex = AEI_INVALID_ASSIGN_ID;

    valueType = (ulId & AEI_MAC_ADDRESS_TYPE_MASK);
    valueNum  = (ulId & AEI_MAC_ADDRESS_NUM_MASK);
    valueId   = (ulId & AEI_MAC_ADDRESS_ID_MASK);

#endif

#if defined(GPL_CODE)
    /* baseMacAddr = last 3 bytes of the base MAC address treated as a 24 bit integer */
    memcpy((unsigned char *) &baseMacAddr,
        &g_pMacInfo->ucaBaseMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    baseMacAddr >>= 8;

#endif

    mutex_lock(&macAddrMutex);

    /* Start with the base address */
    memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr, NVRAM_MAC_ADDRESS_LEN);

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    /*As epon mac should not be dynamicly changed, always use last 1(SLLID) or 8(MLLID) mac address(es)*/
    if (ulId == MAC_ADDRESS_EPONONU)
    {
        i = g_pMacInfo->ulNumMacAddrs - num_addresses; 

        for (j = 0, pMai = &g_pMacInfo->MacAddrs[i]; j < num_addresses; j++) {
            pMaiFreeIdTemp = pMai + j;
            if (pMaiFreeIdTemp->chInUse != 0 && pMaiFreeIdTemp->ulId != MAC_ADDRESS_EPONONU) {
                printk("kerSysGetMacAddresses: epon mac address allocate failed, g_pMacInfo[%ld] reserved by 0x%lx\n", i+j, pMaiFreeIdTemp->ulId);	
                break;
            }
        }
		
        if (j >= num_addresses) {
            pMaiFreeId = pMai;
            ulIdxId = i;
        }
    }
    else
#endif	
    {
#if defined(GPL_CODE_STATIC_MAC_ADDR)
    /*
     *  1. It is used to keep using WAN MAC address is same as before while upgrade TELUS SDK3/SDK6 image to SDK12.
     *  2. SDK3-V1000H/VB784WG, atm0 = B + 1, atm1 = B +4, ptm0 = B + 1, ewan0 = B + 2.
     *  3. SDK6-V2000H, atm0 = B, atm1 = B + 4, ptm0.1 = B + 4, ewan0.1 = B.
     *  4. ptm0/ewan0 is used for using WAN interface in SDK3, but it is ptm0.1/ewan0.1 in SDK12,
     *     So make sure that ptm0/ewan0(sdk3) = ptm0.1/ewan0.1 (SDK12).
     *  5. For other TELUS project (fox example T2200H), my suggestion is that using WAN interface(atm0/ptm0.1/ewan0.1) is B + 1, and atm1 = B + 2.
     *  6. B is base MAC address.
     *  7. If value type is MAC_ADDRESS_ETH, it indicate that WAN Ethernet to request WAN MAC address. Not Lan.
     *  8. In SDK12, eth0-eth4 and ewan0 MAC are set early in ethernet driver and the ulId is low enough that
     *     valueType is 0 and should not come here. ewan0.1 and ptm0.1 are created by the brcm vlanmux module
     *     and the ulId passed had been left-shifted to high number so valueType will hit one of these cases.
     */
    switch (aeiBoardId)
    {
    case AEI_BOARD_V2200H:
    case AEI_BOARD_T2200H:
    case AEI_BOARD_T3200:
        if (valueType == MAC_ADDRESS_ATM)
        {
#if defined(GPL_CODE_VOIP)
            if(ulId == 0xe0003000)
                assignIndex = 3; // atm0.3
            else
                assignIndex = 1; // atm0.x
#else
            assignIndex = 1;// atm0 and atm1
#endif
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
#if defined(GPL_CODE_VOIP)
            if(ulId == 0xf0003000)
                assignIndex = 3; // ptm0.3
            else
                assignIndex = 1; // ptm0.x
#else
            assignIndex = 1;// ptm
#endif
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
#if defined(GPL_CODE_VOIP)
            if(ulId == 0xa0003000)
                assignIndex = 3; // ewan0.3
            else
                assignIndex = 1; // ewan0.x
#else
            assignIndex = 1;// ewan0.x
#endif
        }
        break;
    case AEI_BOARD_V2000H:
        if (valueType == MAC_ADDRESS_ATM)
        {
            if (valueNum == 0) // atm0
                assignIndex = 0;
            else //atm1
                assignIndex = 4;
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
            if (valueId == 0)
                assignIndex = 0;
            else
                assignIndex = 4;
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 0;
        }
        break;
    case AEI_BOARD_V1000H:
        if (valueType == MAC_ADDRESS_ATM)
        {
            if (valueNum == 0) // atm0
                assignIndex = 1;
            else //atm1
                assignIndex = 4;
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
            assignIndex = 1;
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 2;
        }
        break;
    default:
        if (valueType == MAC_ADDRESS_ATM)
        {
            if (valueNum == 0) // atm0
                assignIndex = 1;
            else //atm1
                assignIndex = 2;
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
            assignIndex = 1;
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 1;
        }

        break;
    }

    if ((assignIndex > AEI_INVALID_ASSIGN_ID) && (assignIndex < g_pMacInfo->ulNumMacAddrs))
    {
         getNthMacAddr(pucaMacAddr, assignIndex);
         g_pMacInfo->MacAddrs[assignIndex].ulId = ulId;
         g_pMacInfo->MacAddrs[assignIndex].chInUse = 1;
         mutex_unlock(&macAddrMutex);

         return nRet;
    }
#endif

#if defined(GPL_CODE_SASKTEL_STATIC_MAC_ADDR)
    /*
     *  Keep WAN MAC address same as SDK 16 while updated image  from SDK6 to SDK12
     *  SDK6 WAN MAC list : atm0 = B + 4, atm1 = B + 5;  ptm0.1 = B + 5; ewan0.1 = B + 4;
     */
    switch (aeiBoardId)
    {
    case AEI_BOARD_V1000H:
        if (valueType == MAC_ADDRESS_ATM)
        {
            if (valueNum == 0) // atm0
                assignIndex = 4;
            else //atm1
                assignIndex = 5;
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
            assignIndex = 5;
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 4;
        }
        break;
    default:
        if (valueType == MAC_ADDRESS_ATM)
        {
            if (valueNum == 0) // atm0
                assignIndex = 1;
            else //atm1
                assignIndex = 2;
        }
        else if (valueType == MAC_ADDRESS_PTM)
        {
            assignIndex = 1;
        }
        else if (valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 1;
        }

        break;
    }

    if ((assignIndex > AEI_INVALID_ASSIGN_ID) && (assignIndex < g_pMacInfo->ulNumMacAddrs))
    {
         getNthMacAddr(pucaMacAddr, assignIndex);
         g_pMacInfo->MacAddrs[assignIndex].ulId = ulId;
         g_pMacInfo->MacAddrs[assignIndex].chInUse = 1;
         mutex_unlock(&macAddrMutex);

         return nRet;
    }
#endif

#if defined(GPL_CODE_STATIC_MAC_ADDR)
    switch (aeiBoardId)
    {
    case AEI_BOARD_C3000A:
        if (valueType == MAC_ADDRESS_ATM || valueType == MAC_ADDRESS_PTM || valueType == MAC_ADDRESS_ETH)
        {
            assignIndex = 1;
        }
        break;
    } 

    if(ulId == 0x12ffffff)
    {
            baseMacAddr = (baseMacAddr + 1) << 8;
            memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                constMacAddrIncIndex);
            memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
            g_pMacInfo->MacAddrs[1].ulId = ulId;
            g_pMacInfo->MacAddrs[1].chInUse = 1;
            mutex_unlock(&macAddrMutex);
            return nRet;

    }

    if ((assignIndex > AEI_INVALID_ASSIGN_ID) && (assignIndex < g_pMacInfo->ulNumMacAddrs))
    {
         getNthMacAddr(pucaMacAddr, assignIndex);
         g_pMacInfo->MacAddrs[assignIndex].ulId = ulId;
         g_pMacInfo->MacAddrs[assignIndex].chInUse = 1;
         mutex_unlock(&macAddrMutex);

         return nRet;
    }
#endif

#if defined(GPL_CODE) || defined(GPL_CODE_STATIC_MAC_ADDR)
    if(ulId == 0x13ffffff)
    {
            getNthMacAddr(pucaMacAddr, 2);

            g_pMacInfo->MacAddrs[2].ulId = ulId;
            g_pMacInfo->MacAddrs[2].chInUse = 1;
            mutex_unlock(&macAddrMutex);
            return nRet;

    }
#endif

        for( i = 0, pMai = g_pMacInfo->MacAddrs; i < g_pMacInfo->ulNumMacAddrs;
            i++, pMai++ )
        {
#if defined(GPL_CODE_STATIC_MAC_ADDR)
        /*
         *  1. It is used to preserve mac address.
         *  2. i = 0 (B) could not be preserved, because it is used for LAN ethernet(eth0~eth4).
         */

        if (aeiBoardId == AEI_BOARD_V2200H ||aeiBoardId == AEI_BOARD_T2200H || aeiBoardId == AEI_BOARD_T3200)
        {
#if defined(GPL_CODE_VOIP)
            if (i == 1 || i == 3)
#else
            if (i == 1)
#endif
                continue;
        }
        else if (aeiBoardId == AEI_BOARD_V1000H)
        {
            /*
             *  1. In SDK 3(AEI_BOARD_V1000H), B + 1, B + 2, B + 4 is used for WAN MAC address, so we need to preserve it,
             *     used for static assign the mac address for WAN interface in SDK12.
             *  2. Other except 1, 2, 4 is used to auto assign mac address for other interface, for example usbX, wlX, and so on.
             */

            if ((i == 1) || (i == 2) || (i == 4))
                continue;
        }
        else if (aeiBoardId == AEI_BOARD_V2000H)
        {
            if (i == 4)
                continue;
        }
        else
        {
            if ((i == 1) || (i == 2))
                continue;
        }
#endif

#if defined(GPL_CODE_SASKTEL_STATIC_MAC_ADDR)
        if (aeiBoardId == AEI_BOARD_V1000H)
        {
            if ((i == 4) || (i == 5))
                continue;
        }
        else
        {
            if ((i == 1) || (i == 2))
                continue;
        }
#endif

#if defined(GPL_CODE_STATIC_MAC_ADDR)
        if (i == 1)  /*This mac addr is used for atm0 or ptm0*/
           continue;

        if (i == 2)  /*This mac addr is used for ewan0.* */
           continue;
#endif

#if defined(GPL_CODE)
        if (i == 2)  /*This mac addr is used for ewan0*/
        {
            continue;
        }
#endif

#if defined(BUILD_AEI_QUANTENNA_LIB)
        if (i == AEI_QTN_BSSID_INDEX || i == AEI_QTN_ETH_INDEX)  /*This mac addr is used for Quantenna wifi0*/
        {
            continue;
        }
#endif

            if( ulId == pMai->ulId || ulId == MAC_ADDRESS_ANY )
            {
                /* This MAC address has been used by the caller in the past. */
                getNthMacAddr( pucaMacAddr, i );
                pMai->chInUse = 1;
                pMaiFreeId = NULL;
                nRet = 0;
                break;
            } else if( pMai->chInUse == 0 ) {
                /* check if it there are num_addresses to be checked starting from found MAC address */
                if ((i + num_addresses - 1) >= g_pMacInfo->ulNumMacAddrs) {
                    break;
                }
    
                for (j = 1; j < num_addresses; j++) {
                    pMaiFreeIdTemp = pMai + j;
                    if (pMaiFreeIdTemp->chInUse != 0) {
                        break;
                    }
                }
                if (j == num_addresses) {
                    pMaiFreeId = pMai;
                    ulIdxId = i;
                    break;
                }
            }
        }
    }

    if(pMaiFreeId )
    {
        /* An available MAC address was found. */
        getNthMacAddr( pucaMacAddr, ulIdxId );
        pMaiFreeIdTemp = pMai;
        for (j = 0; j < num_addresses; j++) {
            pMaiFreeIdTemp->ulId = ulId;
            pMaiFreeIdTemp->chInUse = 1;
            pMaiFreeIdTemp++;
        }
        nRet = 0;
    }

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysGetMacAddr */
int kerSysGetMacAddress( unsigned char *pucaMacAddr, unsigned long ulId )
{
    return kerSysGetMacAddresses(pucaMacAddr,1,ulId); /* Get only one address */
} /* kerSysGetMacAddr */


int kerSysReleaseMacAddresses( unsigned char *pucaMacAddr, unsigned int num_addresses )
{
    int i, nRet = -EINVAL;
    unsigned long ulIdx = 0;

    mutex_lock(&macAddrMutex);

    ulIdx = getIdxForNthMacAddr(g_pMacInfo->ucaBaseMacAddr, pucaMacAddr);

    if( ulIdx < g_pMacInfo->ulNumMacAddrs )
    {
        for(i=0; i<num_addresses; i++) {
            if ((ulIdx + i) < g_pMacInfo->ulNumMacAddrs) {
                PMAC_ADDR_INFO pMai = &g_pMacInfo->MacAddrs[ulIdx + i];
                if( pMai->chInUse == 1 )
                {
                    pMai->chInUse = 0;
                    nRet = 0;
                }
            } else {
                printk("Request to release %d addresses failed as "
                    " the one or more of the addresses, starting from"
                    " %dth address from given address, requested for release"
                    " is not in the list of available MAC addresses \n", num_addresses, i);
                break;
            }
        }
    }

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysReleaseMacAddr */


int kerSysReleaseMacAddress( unsigned char *pucaMacAddr )
{
    return kerSysReleaseMacAddresses(pucaMacAddr,1); /* Release only one MAC address */

} /* kerSysReleaseMacAddr */


void kerSysGetGponSerialNumber( unsigned char *pGponSerialNumber )
{
    strcpy( pGponSerialNumber, g_pGponInfo->gponSerialNumber );
}


void kerSysGetGponPassword( unsigned char *pGponPassword )
{
    strcpy( pGponPassword, g_pGponInfo->gponPassword );
}

int kerSysGetSdramSize( void )
{
    return( (int) g_ulSdramSize );
} /* kerSysGetSdramSize */




/*Read Wlan Params data from CFE */
int kerSysGetWlanSromParams( unsigned char *wlanParams, unsigned short len)
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("kerSysGetWlanSromParams: could not read nvram data\n");
        return -1;
    }

    memcpy( wlanParams,
           (char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
            len );
    kfree(pNvramData);

    return 0;
}


unsigned char kerSysGetWlanFeature(void)
{
    NVRAM_DATA *pNvramData;

    unsigned char wlfeature=0;
    if (NULL == (pNvramData = readNvramData()))
    {
        printk("kerSysGetWlanSromParams: could not read nvram data\n");
        return -1;
    }
    wlfeature= (unsigned char)(pNvramData ->wlanParams[NVRAM_WLAN_PARAMS_LEN-1]);
    kfree(pNvramData);
    return wlfeature;
    
}

/*Read Wlan Params data from CFE */
int kerSysGetAfeId( unsigned long *afeId )
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("kerSysGetAfeId: could not read nvram data\n");
        return -1;
    }

    afeId [0] = pNvramData->afeId[0];
    afeId [1] = pNvramData->afeId[1];
    kfree(pNvramData);

    return 0;
}

void kerSysLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    if (g_ledInitialized)
       boardLedCtrl(ledName, ledState);
}

#if defined(GPL_CODE)
void AEI_kerSysLedBrightnessCtrl(BOARD_LED_NAME ledName, unsigned int ledState)
{
    if (g_ledInitialized)
       AEI_boardLedBrightnessCtrl(ledName, ledState);
}
#endif
/*functionto receive message from usersapce
 * Currently we dont expect any messages fromm userspace
 */
void kerSysRecvFrmMonitorTask(struct sk_buff *skb)
{

   /*process the message here*/
   printk(KERN_WARNING "unexpected skb received at %s \n",__FUNCTION__);
   kfree_skb(skb);
   return;
}

void kerSysInitMonitorSocket( void )
{
   g_monitor_nl_sk = netlink_kernel_create(&init_net, NETLINK_BRCM_MONITOR, 0, kerSysRecvFrmMonitorTask, NULL, THIS_MODULE);

   if(!g_monitor_nl_sk)
   {
      printk(KERN_ERR "Failed to create a netlink socket for monitor\n");
      return;
   }

}


void kerSysSendtoMonitorTask(int msgType, char *msgData, int msgDataLen)
{

   struct sk_buff *skb =  NULL;
   struct nlmsghdr *nl_msgHdr = NULL;
   unsigned int nl_msgLen;

   if(!g_monitor_nl_pid)
   {
      printk(KERN_INFO "message received before monitor task is initialized %s \n",__FUNCTION__);
      return;
   } 

   if(msgData && (msgDataLen > MAX_PAYLOAD_LEN))
   {
      printk(KERN_ERR "invalid message len in %s",__FUNCTION__);
      return;
   } 

   nl_msgLen = NLMSG_SPACE(msgDataLen);

   /*Alloc skb ,this check helps to call the fucntion from interrupt context */

   if(in_atomic())
   {
      skb = alloc_skb(nl_msgLen, GFP_ATOMIC);
   }
   else
   {
      skb = alloc_skb(nl_msgLen, GFP_KERNEL);
   }

   if(!skb)
   {
      printk(KERN_ERR "failed to alloc skb in %s",__FUNCTION__);
      return;
   }

   nl_msgHdr = (struct nlmsghdr *)skb->data;
   nl_msgHdr->nlmsg_type = msgType;
   nl_msgHdr->nlmsg_pid=0;/*from kernel */
   nl_msgHdr->nlmsg_len = nl_msgLen;
   nl_msgHdr->nlmsg_flags =0;

   if(msgData)
   {
      memcpy(NLMSG_DATA(nl_msgHdr),msgData,msgDataLen);
   }

   NETLINK_CB(skb).pid = 0; /*from kernel */

   skb->len = nl_msgLen; 

   netlink_unicast(g_monitor_nl_sk, skb, g_monitor_nl_pid, MSG_DONTWAIT);
   return;
}

void kerSysCleanupMonitorSocket(void)
{
   g_monitor_nl_pid = 0 ;
   sock_release(g_monitor_nl_sk->sk_socket);
}

// Must be called with flashImageMutex held
static PFILE_TAG getTagFromPartition(int imageNumber)
{
    // Define space for file tag structures for two partitions.  Make them static
    // so caller does not have to worry about allocation/deallocation.
    // Make sure they're big enough for the file tags plus an block number
    // (an integer) appended.
    static unsigned char sectAddr1[sizeof(FILE_TAG) + sizeof(int)];
    static unsigned char sectAddr2[sizeof(FILE_TAG) + sizeof(int)];

    int blk = 0;
    UINT32 crc;
    PFILE_TAG pTag = NULL;
    unsigned char *pBase = flash_get_memptr(0);
    unsigned char *pSectAddr = NULL;

    unsigned int reserverdBytersAuxfs = flash_get_reserved_bytes_auxfs();
    unsigned int sectSize = (unsigned int) flash_get_sector_size(0);
    unsigned int offset;

    /* The image tag for the first image is always after the boot loader.
     * The image tag for the second image, if it exists, is at one half
     * of the flash size.
     */
    if( imageNumber == 1 )
    {
        // Get the flash info and block number for parition 1 at the base of the flash
        FLASH_ADDR_INFO flash_info;

        kerSysFlashAddrInfoGet(&flash_info);
        blk = flash_get_blk((int)(pBase+flash_info.flash_rootfs_start_offset));
        pSectAddr = sectAddr1;
    }
    else if( imageNumber == 2 )
    {
        // Get block number for partition2 at middle of the device (not counting space for aux
        // file system).
        offset = ((flash_get_total_size()-reserverdBytersAuxfs) / 2);

        /* make sure offset is on sector boundary, image starts on sector boundary */
        if( offset % sectSize )
            offset = ((offset/sectSize)+1)*sectSize;
        blk = flash_get_blk((int) (pBase + offset + IMAGE_OFFSET));

        pSectAddr = sectAddr2;
    }
    
    // Now that you have a block number, use it to read in the file tag
    if( blk )
    {
        int *pn;    // Use to append block number at back of file tag
        
        // Clear out space for file tag structures
        memset(pSectAddr, 0x00, sizeof(FILE_TAG));
        
        // Get file tag
        flash_read_buf((unsigned short) blk, 0, pSectAddr, sizeof(FILE_TAG));
        
        // Figure out CRC of file tag so we can check it below
        crc = CRC32_INIT_VALUE;
        crc = getCrc32(pSectAddr, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        
        // Get ready to return file tag pointer
        pTag = (PFILE_TAG) pSectAddr;
        
        // Append block number after file tag
        pn = (int *) (pTag + 1);
        *pn = blk;
        
        // One final check - if the file tag CRC is not OK, return NULL instead
        if (crc != (UINT32)(*(UINT32*)(pTag->tagValidationToken)))
            pTag = NULL;
    }
    
    // All done - return file tag pointer
    return( pTag );
}

// must be called with flashImageMutex held
static int getPartitionFromTag( PFILE_TAG pTag )
{
    int ret = 0;

    if( pTag )
    {
        PFILE_TAG pTag1 = getTagFromPartition(1);
        PFILE_TAG pTag2 = getTagFromPartition(2);
        int sequence = simple_strtoul(pTag->imageSequence,  NULL, 10);
        int sequence1 = (pTag1) ? simple_strtoul(pTag1->imageSequence, NULL, 10)
            : -1;
        int sequence2 = (pTag2) ? simple_strtoul(pTag2->imageSequence, NULL, 10)
            : -1;

        if( pTag1 && sequence == sequence1 )
            ret = 1;
        else
            if( pTag2 && sequence == sequence2 )
                ret = 2;
    }

    return( ret );
}

// must be called with flashImageMutex held
static PFILE_TAG getBootImageTag(void)
{
    static int displayFsAddr = 1;
    PFILE_TAG pTag = NULL;
    PFILE_TAG pTag1 = getTagFromPartition(1);
    PFILE_TAG pTag2 = getTagFromPartition(2);

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if( pTag1 && pTag2 )
    {
        /* Two images are flashed. */
        int sequence1 = simple_strtoul(pTag1->imageSequence, NULL, 10);
        int sequence2 = simple_strtoul(pTag2->imageSequence, NULL, 10);
        int imgid = 0;

        kerSysBlParmsGetInt(BOOTED_IMAGE_ID_NAME, &imgid);
#if defined(GPL_CODE_DUAL_IMAGE)
        if(sequence1 != IMAGE1_SEQUENCE || sequence2 != IMAGE2_SEQUENCE )
        {
            if( imgid == BOOT_LATEST_IMAGE )
            {
                pTag = pTag1;
            }
            else /* Boot from the second image. */
            {
                pTag = pTag2;
            }
        }
        else
#endif
        if( imgid == BOOTED_OLD_IMAGE )
            pTag = (sequence2 < sequence1) ? pTag2 : pTag1;
        else
            pTag = (sequence2 > sequence1) ? pTag2 : pTag1;
    }
    else
        /* One image is flashed. */
        pTag = (pTag2) ? pTag2 : pTag1;

    if( pTag && displayFsAddr )
    {
        displayFsAddr = 0;
        printk("File system address: 0x%8.8lx\n",
            simple_strtoul(pTag->rootfsAddress, NULL, 10) + BOOT_OFFSET + IMAGE_OFFSET);
    }

    return( pTag );
}

// Must be called with flashImageMutex held
static void UpdateImageSequenceNumber( unsigned char *imageSequence )
{
    int newImageSequence = 0;
    PFILE_TAG pTag = getTagFromPartition(1);

    if( pTag )
        newImageSequence = simple_strtoul(pTag->imageSequence, NULL, 10);

    pTag = getTagFromPartition(2);
    if(pTag && simple_strtoul(pTag->imageSequence, NULL, 10) > newImageSequence)
        newImageSequence = simple_strtoul(pTag->imageSequence, NULL, 10);

    newImageSequence++;
    sprintf(imageSequence, "%d", newImageSequence);
}

/* Must be called with flashImageMutex held */
static int flashFsKernelImage( unsigned char *imagePtr, int imageLen,
    int flashPartition, int *numPartitions )
{

    int status = 0;
    PFILE_TAG pTag = (PFILE_TAG) imagePtr;
    int rootfsAddr = simple_strtoul(pTag->rootfsAddress, NULL, 10);
    int kernelAddr = simple_strtoul(pTag->kernelAddress, NULL, 10);
#if defined(GPL_CODE) && !defined(GPL_CODE_DUAL_IMAGE)
    char *p;
#endif
    char *tagFs = imagePtr;
    unsigned int baseAddr = (unsigned int) flash_get_memptr(0);
    unsigned int totalSize = (unsigned int) flash_get_total_size();
    unsigned int sectSize = (unsigned int) flash_get_sector_size(0);
    unsigned int reservedBytesAtEnd;
    unsigned int reserverdBytersAuxfs;
    unsigned int availableSizeOneImg;
    unsigned int reserveForTwoImages;
    unsigned int availableSizeTwoImgs;
    unsigned int newImgSize = simple_strtoul(pTag->rootfsLen, NULL, 10) +
        simple_strtoul(pTag->kernelLen, NULL, 10);
    PFILE_TAG pCurTag = getBootImageTag();
    int nCurPartition = getPartitionFromTag( pCurTag );
    int should_yield =
        (flashPartition == 0 || flashPartition == nCurPartition) ? 0 : 1;
    UINT32 crc;
    unsigned int curImgSize = 0;
    unsigned int rootfsOffset = (unsigned int) rootfsAddr - IMAGE_BASE - TAG_LEN + IMAGE_OFFSET;
    FLASH_ADDR_INFO flash_info;
    NVRAM_DATA *pNvramData;

#if defined(GPL_CODE_DUAL_IMAGE)
    int newImageSequence = simple_strtoul(pTag->imageSequence, NULL, 10);
#endif

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if (NULL == (pNvramData = readNvramData()))
    {
        return -ENOMEM;
    }

    kerSysFlashAddrInfoGet(&flash_info);
    if( rootfsOffset < flash_info.flash_rootfs_start_offset )
    {
        // Increase rootfs and kernel addresses by the difference between
        // rootfs offset and what it needs to be.
        rootfsAddr += flash_info.flash_rootfs_start_offset - rootfsOffset;
        kernelAddr += flash_info.flash_rootfs_start_offset - rootfsOffset;
        sprintf(pTag->rootfsAddress,"%lu", (unsigned long) rootfsAddr);
        sprintf(pTag->kernelAddress,"%lu", (unsigned long) kernelAddr);
        crc = CRC32_INIT_VALUE;
        crc = getCrc32((unsigned char *)pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        *(unsigned long *) &pTag->tagValidationToken[0] = crc;
    }

    rootfsAddr += BOOT_OFFSET+IMAGE_OFFSET;
    kernelAddr += BOOT_OFFSET+IMAGE_OFFSET;

    reservedBytesAtEnd = flash_get_reserved_bytes_at_end(&flash_info);
    reserverdBytersAuxfs = flash_get_reserved_bytes_auxfs();
    availableSizeOneImg = totalSize - ((unsigned int) rootfsAddr - baseAddr) -
        reservedBytesAtEnd- reserverdBytersAuxfs;  
        
    reserveForTwoImages =
        (flash_info.flash_rootfs_start_offset > reservedBytesAtEnd)
        ? flash_info.flash_rootfs_start_offset : reservedBytesAtEnd;
    availableSizeTwoImgs = ((totalSize-reserverdBytersAuxfs)/ 2) - reserveForTwoImages - sectSize;

    printk("availableSizeOneImage=%dKB availableSizeTwoImgs=%dKB reserverdBytersAuxfs=%dKB reserve=%dKB\n",
        availableSizeOneImg/1024, availableSizeTwoImgs/1024, reserverdBytersAuxfs/1024, reserveForTwoImages/1024);
	   
    if( pCurTag )
    {
        curImgSize = simple_strtoul(pCurTag->rootfsLen, NULL, 10) +
            simple_strtoul(pCurTag->kernelLen, NULL, 10);
    }

    if( newImgSize > availableSizeOneImg)
    {
        printk("Illegal image size %d.  Image size must not be greater "
            "than %d.\n", newImgSize, availableSizeOneImg);
        kfree(pNvramData);
        return -1;
    }

    *numPartitions = (curImgSize <= availableSizeTwoImgs &&
         newImgSize <= availableSizeTwoImgs &&
         flashPartition != nCurPartition) ? 2 : 1;

    // If the current image fits in half the flash space and the new
    // image to flash also fits in half the flash space, then flash it
    // in the partition that is not currently being used to boot from.
    if( curImgSize <= availableSizeTwoImgs &&
        newImgSize <= availableSizeTwoImgs &&
#ifdef GPL_CODE_DUAL_IMAGE
         newImageSequence== IMAGE2_SEQUENCE )
#else
        ((nCurPartition == 1 && flashPartition != 1) || flashPartition == 2) )
#endif
    {
        // Update rootfsAddr to point to the second boot partition.
        int offset = ((totalSize-reserverdBytersAuxfs) / 2);

        /* make sure offset is on sector boundary, image starts on sector boundary */
        if( offset % sectSize )
            offset = ((offset/sectSize)+1)*sectSize;
        offset += TAG_LEN;

        sprintf(((PFILE_TAG) tagFs)->kernelAddress, "%lu",
            (unsigned long) IMAGE_BASE + offset + (kernelAddr - rootfsAddr));
        kernelAddr = baseAddr + offset + (kernelAddr - rootfsAddr) + IMAGE_OFFSET;

        sprintf(((PFILE_TAG) tagFs)->rootfsAddress, "%lu",
            (unsigned long) IMAGE_BASE + offset);
        rootfsAddr = baseAddr + offset + IMAGE_OFFSET;
    }
#ifdef GPL_CODE_DUAL_IMAGE
    memset(((PFILE_TAG) tagFs)->imageSequence,0,sizeof(((PFILE_TAG) tagFs)->imageSequence));
    if(newImageSequence== IMAGE2_SEQUENCE )
    {
        sprintf(((PFILE_TAG) tagFs)->imageSequence,"%d",IMAGE2_SEQUENCE);
    }
    else
        sprintf(((PFILE_TAG) tagFs)->imageSequence,"%d",IMAGE1_SEQUENCE);
#else
    UpdateImageSequenceNumber( ((PFILE_TAG) tagFs)->imageSequence );
#endif
    crc = CRC32_INIT_VALUE;
    crc = getCrc32((unsigned char *)tagFs, (UINT32)TAG_LEN-TOKEN_LEN, crc);
    *(unsigned long *) &((PFILE_TAG) tagFs)->tagValidationToken[0] = crc;

    
    // Now, perform the actual flash write
    if( (status = kerSysBcmImageSet((rootfsAddr-TAG_LEN), tagFs,
        TAG_LEN + newImgSize, should_yield)) != 0 )
    {
        printk("Failed to flash root file system. Error: %d\n", status);
        kfree(pNvramData);
        return status;
    }
    

#if defined(GPL_CODE) && !defined(GPL_CODE_DUAL_IMAGE)
    for( p = pNvramData->szBootline; p[2] != '\0'; p++ )
    {
        if( p[0] == 'p' && p[1] == '=' && p[2] != BOOT_LATEST_IMAGE )
        {
            // Change boot partition to boot from new image.
            p[2] = BOOT_LATEST_IMAGE;
//            writeNvramData(pNvramData);
            writeNvramDataCrcLocked(pNvramData);
            break;
        }
    }
#endif

    // Free buffers
    kfree(pNvramData);
    char ug_info[2]={0};
    kerSysScratchPadGet("UpGrade_Info",ug_info,sizeof(ug_info));
    if(ug_info[0] != '0')
    {
        ug_info[0] = '2';
        kerSysScratchPadSet("UpGrade_Info",ug_info,sizeof(ug_info));
    }
    return(status);
}


#define IMAGE_VERSION_FILE_NAME "/etc/image_version"
#define IMAGE_VERSION_MAX_SIZE  64

static int getImageVersion( int imageNumber, char *verStr, int verStrSize)
{
    static char imageVersions[2][IMAGE_VERSION_MAX_SIZE] = {{'\0'}, {'\0'}};
    int ret = 0; /* zero bytes copied so far */

    if( !((imageNumber == 1 && imageVersions[0][0] != '\0') ||
        (imageNumber == 2 && imageVersions[1][0] != '\0')) )
    {
        /* get up to IMAGE_VERSION_MAX_SIZE and save it in imageVersions[][] */
        unsigned long rootfs_ofs;

        memset(imageVersions[imageNumber - 1], 0, IMAGE_VERSION_MAX_SIZE);
        
        if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
        {
            /* NOR Flash */
            PFILE_TAG pTag = NULL;

            if( imageNumber == 1 )
                pTag = getTagFromPartition(1);
            else
                if( imageNumber == 2 )
                    pTag = getTagFromPartition(2);

            if( pTag )
            {
                /* Save version string for subsequent calls to this function. 
                 MAX length  IMAGE_VER_LEN = 32 bytes */
                memcpy(imageVersions[imageNumber - 1], pTag->imageVersion, IMAGE_VER_LEN);
            }
        }
        else
        {
            /* NAND Flash */
#ifdef GPL_CODE
            PFILE_TAG pTag = NULL;

            if( imageNumber == 1 )
                pTag = getTagFromPartition(1);
            else
                if( imageNumber == 2 )
                    pTag = getTagFromPartition(2);

            if( pTag )
            {
                if( verStrSize > sizeof(pTag->signiture_2) )
                    ret = sizeof(pTag->signiture_2);
                else
                    ret = verStrSize;

                memcpy(verStr, pTag->imageVersion, ret);

                /* Save version string for subsequent calls to this function. */
                memcpy(imageVersions[imageNumber - 1], verStr, ret);
            }
#else
            NVRAM_DATA *pNvramData;

            if( (imageNumber == 1 || imageNumber == 2) &&
                (pNvramData = readNvramData()) != NULL )
            {
                char *pImgVerFileName = NULL;

                mm_segment_t fs;
                struct file *fp;
                int updatePart, getFromCurPart;

                // updatePart is the partition number that is not booted
                // getFromCurPart is 1 to retrieve info from the booted partition
                updatePart =
                    (rootfs_ofs==pNvramData->ulNandPartOfsKb[NP_ROOTFS_1])
                    ? 2 : 1;
                getFromCurPart = (updatePart == imageNumber) ? 0 : 1;

                fs = get_fs();
                set_fs(get_ds());
                if( getFromCurPart == 0 )
                {
                    struct mtd_info *mtd;
                    pImgVerFileName = "/mnt/" IMAGE_VERSION_FILE_NAME;

                    mtd = get_mtd_device_nm("bootfs_update");
                    if( !IS_ERR_OR_NULL(mtd) )
                    {
                        sys_mount("mtd:bootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                        put_mtd_device(mtd);
                    }
                    else
                        sys_mount("mtd:rootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                }
                else
                    pImgVerFileName = IMAGE_VERSION_FILE_NAME;

                fp = filp_open(pImgVerFileName, O_RDONLY, 0);
                if( !IS_ERR(fp) )
                {
                    /* File open successful, read version string from file. */
                    if(fp->f_op && fp->f_op->read)
                    {
                        fp->f_pos = 0;
                        ret = fp->f_op->read(fp, (void *) imageVersions[imageNumber - 1], 
                            IMAGE_VERSION_MAX_SIZE,
                            &fp->f_pos);

                        if (ret > 0)
                        {
                            int i;
                            for (i = 0; i < ret; i ++)
                            {
                                if (imageVersions[imageNumber - 1][i] == 0xa)//line feed
                                {
                                    imageVersions[imageNumber - 1][i] = '\0';//end
                                    ret = i+1;
                                    break;
                                }
                            }
                        }
                    }
                    
                    filp_close(fp, NULL);
                }

                if( getFromCurPart == 0 )
                    sys_umount("/mnt", 0);

                set_fs(fs);
                kfree(pNvramData);
            }
#endif
        }
    }
    
    /* copy the first verStrSize bytes of the stored version to the caller's buffer */
    if( verStrSize > IMAGE_VERSION_MAX_SIZE )
        ret = IMAGE_VERSION_MAX_SIZE;
    else
        ret = verStrSize;
    memcpy(verStr, imageVersions[imageNumber - 1], ret);

    return( ret );
}

PFILE_TAG kerSysUpdateTagSequenceNumber(int imageNumber)
{
    PFILE_TAG pTag = NULL;
    UINT32 crc;

    switch( imageNumber )
    {
    case 0:
        pTag = getBootImageTag();
        break;

    case 1:
        pTag = getTagFromPartition(1);
        break;

    case 2:
        pTag = getTagFromPartition(2);
        break;

    default:
        break;
    }

    if( pTag )
    {
        UpdateImageSequenceNumber( pTag->imageSequence );
        crc = CRC32_INIT_VALUE;
        crc = getCrc32((unsigned char *)pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        *(unsigned long *) &pTag->tagValidationToken[0] = crc;
    }

    return(pTag);
}

int kerSysGetSequenceNumber(int imageNumber)
{
    int seqNumber = -1;
    unsigned long rootfs_ofs;
    if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
    {
        /* NOR Flash */
        PFILE_TAG pTag = NULL;

        switch( imageNumber )
        {
        case 0:
            pTag = getBootImageTag();
            break;

        case 1:
            pTag = getTagFromPartition(1);
            break;

        case 2:
            pTag = getTagFromPartition(2);
            break;

        default:
            break;
        }

        if( pTag )
            seqNumber= simple_strtoul(pTag->imageSequence, NULL, 10);
    }
    else
    {
        /* NAND Flash */
        NVRAM_DATA *pNvramData;

        if( (pNvramData = readNvramData()) != NULL )
        {
            char fname[] = NAND_CFE_RAM_NAME;
            char cferam_buf[32], cferam_fmt[32]; 
            int i;

            mm_segment_t fs;
            struct file *fp;
            int updatePart, getFromCurPart;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96848)
            /* If full secure boot is in play, the CFE RAM file is the encrypted version */
            int boot_secure = otp_is_boot_secure();
            if (boot_secure)
               strcpy(fname, NAND_CFE_RAM_SECBT_NAME);
#endif

            // updatePart is the partition number that is not booted
            // getFromCurPart is 1 to retrieive info from the booted partition
            updatePart = (rootfs_ofs==pNvramData->ulNandPartOfsKb[NP_ROOTFS_1])
                ? 2 : 1;
            getFromCurPart = (updatePart == imageNumber) ? 0 : 1;

            fs = get_fs();
            set_fs(get_ds());
            if( getFromCurPart == 0 )
            {
                struct mtd_info *mtd;
                strcpy(cferam_fmt, "/mnt/");
                mtd = get_mtd_device_nm("bootfs_update");
                if( !IS_ERR_OR_NULL(mtd) )
                {
                    sys_mount("mtd:bootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                    put_mtd_device(mtd);
                }
                else
                    sys_mount("mtd:rootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
            }
            else
            {
                struct mtd_info *mtd;
                mtd = get_mtd_device_nm("bootfs");
                if( !IS_ERR_OR_NULL(mtd) )
                {
                    strcpy(cferam_fmt, "/bootfs/");
                    put_mtd_device(mtd);
                }
                else
                    cferam_fmt[0] = '\0';
            }

            /* Find the sequence number of the specified partition. */
            fname[strlen(fname) - 3] = '\0'; /* remove last three chars */
            strcat(cferam_fmt, fname);
            strcat(cferam_fmt, "%3.3d");

            for( i = 0; i < 999; i++ )
            {
                sprintf(cferam_buf, cferam_fmt, i);
                fp = filp_open(cferam_buf, O_RDONLY, 0);
                if (!IS_ERR(fp) )
                {
                    filp_close(fp, NULL);

                    /* Seqence number found. */
                    seqNumber = i;
                    break;
                }
            }

            if( getFromCurPart == 0 )
                sys_umount("/mnt", 0);

            set_fs(fs);
            kfree(pNvramData);
        }
    }

    return(seqNumber);
}

static int getBootedValue(int getBootedPartition)
{
    static int s_bootedPartition = -1;
    int ret = -1;
    int imgId = -1;

    kerSysBlParmsGetInt(BOOTED_IMAGE_ID_NAME, &imgId);

    /* The boot loader parameter will only be "new image", "old image" or "only
     * image" in order to be compatible with non-OMCI image update. If the
     * booted partition is requested, convert this boot type to partition type.
     */
    if( imgId != -1 )
    {
        if( getBootedPartition )
        {
            if( s_bootedPartition != -1 )
                ret = s_bootedPartition;
            else
            {
                /* Get booted partition. */
                int seq1 = kerSysGetSequenceNumber(1);
                int seq2 = kerSysGetSequenceNumber(2);

                switch( imgId )
                {
                case BOOTED_NEW_IMAGE:
                    if( seq1 == -1 || seq2 > seq1 )
                        ret = BOOTED_PART2_IMAGE;
                    else
                        if( seq2 == -1 || seq1 >= seq2 )
                            ret = BOOTED_PART1_IMAGE;
                    break;

                case BOOTED_OLD_IMAGE:
                    if( seq1 == -1 || seq2 < seq1 )
                        ret = BOOTED_PART2_IMAGE;
                    else
                        if( seq2 == -1 || seq1 <= seq2 )
                            ret = BOOTED_PART1_IMAGE;
                    break;

                case BOOTED_ONLY_IMAGE:
                    ret = (seq1 == -1) ? BOOTED_PART2_IMAGE : BOOTED_PART1_IMAGE;
                    break;

                default:
                    break;
                }

                s_bootedPartition = ret;
            }
        }
        else
            ret = imgId;
    }
    return( ret );
}


#if !defined(CONFIG_BRCM_IKOS)
PFILE_TAG kerSysImageTagGet(void)
{
    PFILE_TAG tag;

    mutex_lock(&flashImageMutex);
    tag = getBootImageTag();
    mutex_unlock(&flashImageMutex);

    return tag;
}
#else
PFILE_TAG kerSysImageTagGet(void)
{
    return( (PFILE_TAG) (FLASH_BASE + FLASH_LENGTH_BOOT_ROM));
}
#endif

/*
 * Common function used by BCM_IMAGE_CFE and BCM_IMAGE_WHOLE ioctls.
 * This function will acquire the flashImageMutex
 *
 * @return 0 on success, -1 on failure.
 */
static int commonImageWrite(int flash_start_addr, char *string, int size,
    int *pnoReboot, int partition)
{
    NVRAM_DATA * pNvramDataOrig;
    NVRAM_DATA * pNvramDataNew=NULL;
    int ret;


    mutex_lock(&flashImageMutex);

    // Get a copy of the nvram before we do the image write operation
    if (NULL != (pNvramDataOrig = readNvramData()))
    {
        unsigned long rootfs_ofs;

#if defined(GPL_CODE_CONFIG_JFFS)
        unsigned char * pBuf = NULL;
	    Bool cfeImage = 0;
#endif

        if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
        {
            /* NOR flash */
            ret = kerSysBcmImageSet(flash_start_addr, string, size, 0);
        }
        else
        {
            /* NAND flash */
            char *rootfs_part = "image_update";

#if defined(GPL_CODE_CONFIG_JFFS)
            if( partition == 3 )
            {
               /*dual reboot it after both image flashing finished. set noreboot firstly*/
                if( !pnoReboot )
                    *pnoReboot = 1;

                pBuf = string;

                if((*(unsigned short *) string != JFFS2_MAGIC_BITMASK) 
                    && (*(unsigned short *) (string + 2) != AEI_MAGIC_BITMASK) )
                {
                    /* it is cfe image */
                    cfeImage = 1;
                }
            }
            else
#endif
            if( partition && rootfs_ofs == pNvramDataOrig->ulNandPartOfsKb[
                NP_ROOTFS_1 + partition - 1] )
            {
                /* The image to be flashed is the booted image. Force board
                 * reboot.
                 */
                rootfs_part = "image";
                if( pnoReboot )
                    *pnoReboot = 0;
            }

            ret = kerSysBcmNandImageSet(rootfs_part, string, size,
                (pnoReboot) ? *pnoReboot : 0);

#if defined(GPL_CODE_CONFIG_JFFS)
            if( partition == 3 )
            {
               /*need reboot after the second partitions flashed*/
               *pnoReboot = 0; 
               rootfs_part = "image";
   
               printk("begin flashing partition %s\n", rootfs_part);

               if ( cfeImage )
               { 
                  /*needn't write cfe when flashing dual partition the second time*/
                  pBuf += pNvramDataOrig->ulNandPartSizeKb[NP_BOOT]*1024;
                  size -= pNvramDataOrig->ulNandPartSizeKb[NP_BOOT]*1024;
               }

               ret = kerSysBcmNandImageSet(rootfs_part, pBuf, size,
                   (pnoReboot) ? *pnoReboot : 0);
            }
#endif
        }

        /*
         * After the image is written, check the nvram.
         * If nvram is bad, write back the original nvram.
         */
        pNvramDataNew = readNvramData();
        if ((0 != ret) ||
            (NULL == pNvramDataNew) ||
            (BpSetBoardId(pNvramDataNew->szBoardId) != BP_SUCCESS)
#if defined (CONFIG_BCM_ENDPOINT_MODULE)
            || (BpSetVoiceBoardId(pNvramDataNew->szVoiceBoardId) != BP_SUCCESS)
#endif
            )
        {
            // we expect this path to be taken.  When a CFE or whole image
            // is written, it typically does not have a valid nvram block
            // in the image.  We detect that condition here and restore
            // the previous nvram settings.  Don't print out warning here.
            writeNvramDataCrcLocked(pNvramDataOrig);

            // don't modify ret, it is return value from kerSysBcmImageSet
        }
       
        if(ret == 0)
        {
             char ug_info[2]={0};
             kerSysScratchPadGet("UpGrade_Info",ug_info,sizeof(ug_info));
             if(ug_info[0] != '0')
             {
                  ug_info[0] = '2';
                  kerSysScratchPadSet("UpGrade_Info",ug_info,sizeof(ug_info));
              }
         }

    }
    else
    {
        ret = -1;
    }

    mutex_unlock(&flashImageMutex);

    if (pNvramDataOrig)
        kfree(pNvramDataOrig);
    if (pNvramDataNew)
        kfree(pNvramDataNew);

    return ret;
}

struct file_operations monitor_fops;

#if defined(HAVE_UNLOCKED_IOCTL)
static long board_unlocked_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct inode *inode;
    long rt;
    
    inode = filep->f_dentry->d_inode;

    mutex_lock(&ioctlMutex);
    rt = board_ioctl( inode, filep, cmd, arg );
    mutex_unlock(&ioctlMutex);
    return rt;
    
}
#endif

#if defined(GPL_CODE)

/**************************************** 
 * Arguments:
 *   pIndex   : the index of NVRAM's  param
 *   parVaule : the value of NVRAM's  param
 *   len      : the size of parValue's array
 *
 * Purpose :
 *   This function is used to get the value of NVRAM's param by the param's index
 *
 * Return :
 *    0 : success
 *   -1 : fail
 ****************************************/
static int aei_getNvramValueByParaName(AEI_NVRAM_PARAM_INDEX pIndex, char* parValue, int len)
{
    NVRAM_DATA nvramData;

    if (parValue == NULL || len == 0)
        return -1;

    if (AEI_readNvramData(&nvramData) != 0)
        return -1;

    switch (pIndex)
    {
    case AEI_NVPARAM_SN:
        strlcpy(parValue, nvramData.ulSerialNumber, len);
        break;
    case AEI_NVPARAM_FW:
        strlcpy(parValue, nvramData.chFactoryFWVersion, len);
        break;
    case AEI_NVPARAM_WPS_PIN:
        strlcpy(parValue, nvramData.wpsPin, len);
        break;
    case AEI_NVPARAM_WPA_KEY:
        strlcpy(parValue, nvramData.wpaKey, len);
        break;
    case AEI_NVPARAM_ADMIN_PW:
        strlcpy(parValue, nvramData.adminPassword, len);
        break;
    case AEI_NVPARAM_BOOTLINE:
        strlcpy(parValue, nvramData.szBootline, len);
        break;
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
    case AEI_NVPARAM_WLAN_FEATURE:
        snprintf(parValue, len, "0x%02x\0",(unsigned char)nvramData.wlanParams[len - 1]);
        break;
#endif
    case AEI_NVPARAM_HW_VERSION:
        strlcpy(parValue, nvramData.HwVersion, len);
        break;
#ifdef GPL_CODE_FACTORY_TEST
    case AEI_NVPARAM_BURN_IN_TEST:
        strlcpy(parValue, nvramData.BurnInTest, len);
        break;
    case AEI_NVPARAM_MANU_MODE:
        strlcpy(parValue, nvramData.ManuMode, len);
        break;
    case AEI_NVPARAM_ENABLE_BACKUP_PSI:
        *parValue = nvramData.backupPsi;
        break;
    case AEI_NVPARAM_PSI_SIZE:
        *(unsigned int *)parValue = nvramData.ulPsiSize;
        break;
    case AEI_NVPARAM_SYSTEM_LOG_SIZE:
        *(unsigned int *)parValue = nvramData.ulSyslogSize;
        break;
    case AEI_NVPARAM_AUXILLARY_FILE_SYSTEM_SIZE:
        *parValue = nvramData.ucAuxFSPercent;
        break;
    case AEI_NVPARAM_MAIN_THREAD_NUMBER:
        *(unsigned int *)parValue = nvramData.ulMainTpNum;
        break;
    case AEI_NVPARAM_NUMBER_MAC_ADDRESSES:
        *(unsigned int *)parValue = nvramData.ulNumMacAddrs;
        break;
    case AEI_NVPARAM_WLANFEATURE:
        *parValue = nvramData.wlanParams[NVRAM_WLAN_PARAMS_LEN-1];
        break;
#if defined(GPL_CODE_DETECT_BOARD_ID)
    case AEI_NVPARAM_DETECT_BID:
        sprintf(parValue, "%d", nvramData.detectBID);
        break;
#endif
    case AEI_NVPARAM_BASE_MAC_ADDRESS:
        memcpy(parValue, nvramData.ucaBaseMacAddr, len);
        break;
    case AEI_NVPARAM_VOICE_BOARD_ID:
        memcpy(parValue, nvramData.szVoiceBoardId, len);
        break;
    case AEI_NVPARAM_FACTORY_FW_VERSION:
        memcpy(parValue, nvramData.chFactoryFWVersion, len);
        break;
    case AEI_NVPARAM_RDP_TM_SIZE:
        *(unsigned char*)parValue = nvramData.allocs.alloc_rdp.tmsize;
        break;
    case AEI_NVPARAM_RDP_MC_SIZE:
        *(unsigned char*)parValue = nvramData.allocs.alloc_rdp.mcsize;
        break;
    case AEI_NVPARAM_DHD_MEM_0_ZISE:
        *(unsigned char*)parValue = nvramData.alloc_dhd.dhd_size[0];
        break;
    case AEI_NVPARAM_DHD_MEM_1_ZISE:
        *(unsigned char*)parValue = nvramData.alloc_dhd.dhd_size[1];
        break;
    case AEI_NVPARAM_DHD_MEM_2_ZISE:
        *(unsigned char*)parValue = nvramData.alloc_dhd.dhd_size[2];
        break;
#endif
    default:
        break;
    }

    return 0;
}
#endif

#if defined(GPL_CODE_FACTORY_TEST)
unsigned int isInManuMode(void)
{
    char manuMode[2] = {0};
    aei_getNvramValueByParaName(AEI_NVPARAM_MANU_MODE, manuMode, sizeof(manuMode));
    return manuMode[0] == '1' ? 1 : 0;
}
#endif


//********************************************************************************************
// misc. ioctl calls come to here. (flash, led, reset, kernel memory access, etc.)
//********************************************************************************************
static int board_ioctl( struct inode *inode, struct file *flip,
                       unsigned int command, unsigned long arg )
{
    int ret = 0;
    BOARD_IOCTL_PARMS ctrlParms;
    unsigned char ucaMacAddr[NVRAM_MAC_ADDRESS_LEN];

    switch (command) {
#if defined(BRCM_XDSL_DISTPOINT)
    case BOARD_IOCTL_FTTDP_DSP_BOOTER:	
        download_dsp_booter();
        break;
#endif
    case BOARD_IOCTL_FLASH_WRITE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {

            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                if (ctrlParms.offset == -1)
                    ret =  kerSysScratchPadClearAll();
                else
                    ret = kerSysScratchPadSet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
                break;

            case PERSISTENT:
                ret = kerSysPersistentSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case BACKUP_PSI:
                ret = kerSysBackupPsiSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
            case CUSTOMER_PSI:
                ret = kerSysCustomerPsiSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#endif
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
           case AEI_IMAGE1_PSI:
           
           case AEI_IMAGE2_PSI:
                ret = AEI_kerSysImagePsiSet(ctrlParms.action,ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
            case OLD_IMAGE_PSI:
                ret = kerSysOldImageCfgSet(ctrlParms.string, ctrlParms.strLen);
                break;
#endif
            case SYSLOG:
                ret = kerSysSyslogSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#if defined(GPL_CODE)
            case SYSLOGONREBOOT:
                ret = AEI_SaveSyslogOnReboot();
                break;
#endif
            case NVRAM:
            {
                NVRAM_DATA * pNvramData;

                /*
                 * Note: even though NVRAM access is protected by
                 * flashImageMutex at the kernel level, this protection will
                 * not work if two userspaces processes use ioctls to get
                 * NVRAM data, modify it, and then use this ioctl to write
                 * NVRAM data.  This seems like an unlikely scenario.
                 */
                mutex_lock(&flashImageMutex);
                if (NULL == (pNvramData = readNvramData()))
                {
                    mutex_unlock(&flashImageMutex);
                    return -ENOMEM;
                }
                if ( !strncmp(ctrlParms.string, "WLANFEATURE", 11 ) ) { //Wlan Data data
                    pNvramData->wlanParams[NVRAM_WLAN_PARAMS_LEN-1]= *(unsigned char *)(ctrlParms.string+11);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, "WLANDATA", 8 ) ) { //Wlan Data data
                    int t_strlen=ctrlParms.strLen-8;
                    if(t_strlen>NVRAM_WLAN_PARAMS_LEN-1)
                        t_strlen=NVRAM_WLAN_PARAMS_LEN-1;
                    memset((char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
                        0, sizeof(pNvramData->wlanParams)-1 );
                    memcpy( (char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
                        ctrlParms.string+8,
                        t_strlen);
                    writeNvramDataCrcLocked(pNvramData);
                }
#ifdef GPL_CODE_DETECT_BOARD_ID
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_AUTO_DETECT_BID_TAG, sizeof(NVRAM_AEI_AUTO_DETECT_BID_TAG) ) ) { //detect board id flag
                    pNvramData->detectBID= *(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_AUTO_DETECT_BID_TAG))-'0';
                    writeNvramDataCrcLocked(pNvramData);
                }
#endif
#if defined (GPL_CODE)
                //SUPPORT_DSL_BONDING macro not carried here so leave out since non-bonding will not call this anyways
                else if (ctrlParms.string && !strncmp(ctrlParms.string, "DSLDATAPUMP", 11)) {
                    if (strlen(ctrlParms.string) > 11)
                        pNvramData->dslDatapump = (unsigned long) simple_strtol(ctrlParms.string+11, NULL, 10);
                    else
                        pNvramData->dslDatapump = 0;

                    writeNvramDataCrcLocked(pNvramData);
                }
#ifdef GPL_CODE_FACTORY_TEST
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_BURN_IN_TEST_TAG, sizeof(NVRAM_AEI_BURN_IN_TEST_TAG) ) ) { //Burn in test
                    pNvramData->BurnInTest[0]= *(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_BURN_IN_TEST_TAG));
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_MANU_MODE_TAG, sizeof(NVRAM_AEI_MANU_MODE_TAG) ) ) { //Manu mode
		    if (pNvramData->ManuMode[0] != *(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_MANU_MODE_TAG)))
                    {
                        pNvramData->ManuMode[0]= *(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_MANU_MODE_TAG));
                        writeNvramDataCrcLocked(pNvramData);
		    }
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_WPA_KEY_TAG, sizeof(NVRAM_AEI_WPA_KEY_TAG) ) ){// wpa key
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_WPA_KEY_TAG);
                    if(t_strlen>sizeof(pNvramData->wpaKey)-1)
                        t_strlen=sizeof(pNvramData->wpaKey)-1;
                   memset(pNvramData->wpaKey,0,sizeof(pNvramData->wpaKey));
                   //printk("\npNvramData-wpaKey=%s.t_strlen=%d ,ctlParms.string=%s\n\n",pNvramData->wpaKey,t_strlen,ctrlParms.string+sizeof(NVRAM_AEI_WPA_KEY_TAG));
                   memcpy(pNvramData->wpaKey,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_WPA_KEY_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_WPS_AP_PIN_TAG, sizeof(NVRAM_AEI_WPS_AP_PIN_TAG) ) ){ // wps ap pin
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_WPS_AP_PIN_TAG);
                    if(t_strlen>sizeof(pNvramData->wpsPin)-1)
                        t_strlen=sizeof(pNvramData->wpsPin)-1;
                   memset(pNvramData->wpsPin,0,sizeof(pNvramData->wpsPin));
                   //printk("\npNvramData-wpsPin=%s.t_strlen=%d ,ctlParms.string=%s\n\n",pNvramData->wpsPin,t_strlen,ctrlParms.string+sizeof(NVRAM_AEI_WPS_AP_PIN_TAG));
                   memcpy(pNvramData->wpsPin,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_WPS_AP_PIN_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_ADMIN_PASSWORD_TAG, sizeof(NVRAM_AEI_ADMIN_PASSWORD_TAG) ) ){// admin password
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_ADMIN_PASSWORD_TAG);
                    if(t_strlen>sizeof(pNvramData->adminPassword)-1)
                        t_strlen=sizeof(pNvramData->adminPassword)-1;
                   memset(pNvramData->adminPassword,0,sizeof(pNvramData->adminPassword));
                  // printk("\npNvramData-adminPassword=%s.t_strlen=%d ,ctlParms.string=%s\n\n",pNvramData->adminPassword,t_strlen,ctrlParms.string+sizeof(NVRAM_AEI_ADMIN_PASSWORD_TAG));
                   memcpy(pNvramData->adminPassword,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_ADMIN_PASSWORD_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_SN_TAG, sizeof(NVRAM_AEI_SN_TAG) ) ){ //  board serial number
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_SN_TAG);
                    if(t_strlen>sizeof(pNvramData->ulSerialNumber)-1)
                        t_strlen=sizeof(pNvramData->ulSerialNumber)-1;
                   memset(pNvramData->ulSerialNumber,0,sizeof(pNvramData->ulSerialNumber));
                   //printk("\npNvramData-ulSerialNumber=%s.t_strlen=%d ,ctlParms.string=%s\n\n",pNvramData->ulSerialNumber,t_strlen,ctrlParms.string+sizeof(NVRAM_AEI_ADMIN_PASSWORD_TAG));
                   memcpy(pNvramData->ulSerialNumber,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_SN_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_HW_VERSION_TAG, sizeof(NVRAM_AEI_HW_VERSION_TAG) ) ){ //hw version
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_HW_VERSION_TAG);
                    if(t_strlen>sizeof(pNvramData->HwVersion)-1)
                        t_strlen=sizeof(pNvramData->HwVersion)-1;
                   memset(pNvramData->HwVersion,0,sizeof(pNvramData->HwVersion));
                   //printk("\npNvramData-HwVersion=%s.t_strlen=%d ,ctlParms.string=%s\n\n",pNvramData->HwVersion,t_strlen,ctrlParms.string+sizeof(NVRAM_AEI_HW_VERSION_TAG));
                   memcpy(pNvramData->HwVersion,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_HW_VERSION_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_NUMBER_MAC_ADDRESS_TAG, sizeof(NVRAM_AEI_NUMBER_MAC_ADDRESS_TAG) ) ) { //number mac addresses
                    //printk("\npNvramData-ulNumMacAddrs=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_NUMBER_MAC_ADDRESS_TAG)));
                    pNvramData->ulNumMacAddrs=simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_NUMBER_MAC_ADDRESS_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if( !strncmp(ctrlParms.string, NVRAM_AEI_BASE_MAC_ADDRESS_TAG, sizeof(NVRAM_AEI_BASE_MAC_ADDRESS_TAG) )){//base  mac address
                   int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_BASE_MAC_ADDRESS_TAG);
                    if(t_strlen>sizeof(pNvramData->ucaBaseMacAddr))
                        t_strlen=sizeof(pNvramData->ucaBaseMacAddr);
#if 0
                   {
                       unsigned char *oldMac = (unsigned char *)pNvramData->ucaBaseMacAddr;
                       unsigned char *newMac = (unsigned char *)ctrlParms.string+sizeof(NVRAM_AEI_BASE_MAC_ADDRESS_TAG);
                   printk("\nold BaseMacAddr=%2x:%2x:%2x:%2x:%2x:%2x, t_strlen=%d, new BaseMacAddr=%2x:%2x:%2x:%2x:%2x:%2x\n",
                          oldMac[0], oldMac[1], oldMac[2], oldMac[3], oldMac[4], oldMac[5], t_strlen,
                          newMac[0], newMac[1], newMac[2], newMac[3], newMac[4], newMac[5]);
                   }
#endif
                   memset(pNvramData->ucaBaseMacAddr,0,sizeof(pNvramData->ucaBaseMacAddr));
                   memcpy(pNvramData->ucaBaseMacAddr,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_BASE_MAC_ADDRESS_TAG)),t_strlen);
                   writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_PSI_SIZE_TAG, sizeof(NVRAM_AEI_PSI_SIZE_TAG) ) ) { //PSI size
                    //printk("\npNvramData-ulPsiSize=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_PSI_SIZE_TAG)));
                    pNvramData->ulPsiSize=simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_PSI_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_BACKUP_PSI_TAG, sizeof(NVRAM_AEI_BACKUP_PSI_TAG) ) ) { //enable/disable backup psi
                    //printk("\npNvramData-backupPsi=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_BACKUP_PSI_TAG)));
                    pNvramData->backupPsi=(char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_BACKUP_PSI_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_SYSTEM_LOG_SIZE_TAG, sizeof(NVRAM_AEI_SYSTEM_LOG_SIZE_TAG) ) ) { //system log size
                    //printk("\npNvramData-ulSyslogSize=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_SYSTEM_LOG_SIZE_TAG)));
                    pNvramData->ulSyslogSize=simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_SYSTEM_LOG_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_FACTORY_FW_VERSION_TAG, sizeof(NVRAM_AEI_FACTORY_FW_VERSION_TAG) ) ) { //factory FW version
                    int t_strlen=ctrlParms.strLen-sizeof(NVRAM_AEI_FACTORY_FW_VERSION_TAG);
                    if(t_strlen>sizeof(pNvramData->chFactoryFWVersion)-1)
                        t_strlen=sizeof(pNvramData->chFactoryFWVersion)-1;
                    memset(pNvramData->chFactoryFWVersion,0,sizeof(pNvramData->chFactoryFWVersion));
                    //printk("\npNvramData-chFactoryFWVersion=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_FACTORY_FW_VERSION_TAG)));
                    memcpy(pNvramData->chFactoryFWVersion,(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_FACTORY_FW_VERSION_TAG)),t_strlen);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_WLAN_FEATURE_TAG, sizeof(NVRAM_AEI_WLAN_FEATURE_TAG) ) ) { //set wlan feature
                    //printk("\npNvramData-wlanfeature=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_WLAN_FEATURE_TAG)));
                    if ((unsigned char)pNvramData->wlanParams[NVRAM_WLAN_PARAMS_LEN-1] != (char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_WLAN_FEATURE_TAG)),NULL,0))
		    {
                        pNvramData->wlanParams[NVRAM_WLAN_PARAMS_LEN-1]=(char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_WLAN_FEATURE_TAG)),NULL,0);
                        writeNvramDataCrcLocked(pNvramData);
		    }
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_RDP_TM_SIZE_TAG, sizeof(NVRAM_AEI_RDP_TM_SIZE_TAG) ) ) { //config rdp tm size
                    //printk("\npNvramData-tm-size=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_RDP_TM_SIZE_TAG)));
                    pNvramData->allocs.alloc_rdp.tmsize=(unsigned char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_RDP_TM_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_RDP_MC_SIZE_TAG, sizeof(NVRAM_AEI_RDP_MC_SIZE_TAG) ) ) { //config rdp mc size
                    //printk("\npNvramData-mc-size=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_RDP_MC_SIZE_TAG)));
                    pNvramData->allocs.alloc_rdp.mcsize=(unsigned char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_RDP_MC_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_DHD_MEM_0_SIZE_TAG, sizeof(NVRAM_AEI_DHD_MEM_0_SIZE_TAG) ) ) { //config dhd 0 mem size
                    //printk("\npNvramData-dhd-0-size=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_0_SIZE_TAG)));
                    pNvramData->alloc_dhd.dhd_size[0]=(unsigned char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_0_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_DHD_MEM_1_SIZE_TAG, sizeof(NVRAM_AEI_DHD_MEM_1_SIZE_TAG) ) ) { //config dhd 1 mem size
                    //printk("\npNvramData-dhd-1-size=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_1_SIZE_TAG)));
                    pNvramData->alloc_dhd.dhd_size[1]=(unsigned char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_1_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else if ( !strncmp(ctrlParms.string, NVRAM_AEI_DHD_MEM_2_SIZE_TAG, sizeof(NVRAM_AEI_DHD_MEM_2_SIZE_TAG) ) ) { //config dhd 2 mem size
                    //printk("\npNvramData-dhd-2-size=%s\n",(unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_2_SIZE_TAG)));
                    pNvramData->alloc_dhd.dhd_size[2]=(unsigned char)simple_strtol((unsigned char *)(ctrlParms.string+sizeof(NVRAM_AEI_DHD_MEM_2_SIZE_TAG)),NULL,0);
                    writeNvramDataCrcLocked(pNvramData);
                }

#endif
#endif

#if defined(GPL_CODE_CONSOLE_SECURITY_MODE)
                else if (ctrlParms.string && !strncmp(ctrlParms.string, "CONSOLE", strlen("CONSOLE"))) {
                    if (pNvramData->consoleAccess != (char)ctrlParms.offset) {
                        pNvramData->consoleAccess = (char)ctrlParms.offset;
                        writeNvramDataCrcLocked(pNvramData);
                        printk("board ioctl set console access from %d to %d\n", pNvramData->consoleAccess, ctrlParms.offset);
                    } else
                        printk("nvram data is not changed, needn't update console access\n");
                }

                else if (ctrlParms.string && !strncmp(ctrlParms.string, "CFEPASSWORD", strlen("CFEPASSWORD"))) {
                    if (strlen(ctrlParms.buf) > sizeof(pNvramData->cfePassword) - 1) {
                        printk("board ioctl password is too long: maxlen: %d, but real len: %d\n",
                                    sizeof(pNvramData->cfePassword) - 1, strlen(ctrlParms.buf));
                        ret = -1;
                    } else if (strncmp(pNvramData->cfePassword, ctrlParms.buf, sizeof(pNvramData->cfePassword))) {
                        strncpy(pNvramData->cfePassword, ctrlParms.buf, sizeof(pNvramData->cfePassword));
                        writeNvramDataCrcLocked(pNvramData);
                        printk("board ioctl set cfe password seccess\n");
                    } else
                        printk("nvram data is not changed, needn't update cfe password\n");
                }
#endif
#if defined(GPL_CODE_SECURITY_LEVEL_3)
#if defined(GPL_CODE_SECURITY_LEVEL_3_TR)
                else if (ctrlParms.string && !strncmp(ctrlParms.string, "SENSITIVE_FLAG", strlen("SENSITIVE_FLAG"))) {
                    if (pNvramData->sensitive_flag != (unsigned char)ctrlParms.offset) {
                        pNvramData->sensitive_flag = (unsigned char)ctrlParms.offset;
                        writeNvramDataCrcLocked(pNvramData);
                        printk("board ioctl set sensitive_flag from %d to %d\n", pNvramData->sensitive_flag, ctrlParms.offset);
                    } else
                        printk("nvram data is not changed, needn't update sensitive_flag\n");
                }
#endif
#endif
                else {
                    // assumes the user has calculated the crc in the nvram struct
                    ret = kerSysNvRamSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                }
                mutex_unlock(&flashImageMutex);
                kfree(pNvramData);
                break;
            }

            case BCM_IMAGE_CFE:
                {
                unsigned long not_used;

                if(kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *)&not_used)==0)
                {
                    printk("\nERROR: Image does not support a NAND flash device.\n\n");
                    ret = -1;
                    break;
                }
                if( ctrlParms.strLen <= 0 || ctrlParms.strLen > FLASH_LENGTH_BOOT_ROM )
                {
                    printk("Illegal CFE size [%d]. Size allowed: [%d]\n",
                        ctrlParms.strLen, FLASH_LENGTH_BOOT_ROM);
                    ret = -1;
                    break;
                }

                ret = commonImageWrite(ctrlParms.offset + BOOT_OFFSET + IMAGE_OFFSET,
                    ctrlParms.string, ctrlParms.strLen, NULL, 0);

                }
                break;

            case BCM_IMAGE_FS:
                {
                int numPartitions = 1;
                int noReboot = FLASH_IS_NO_REBOOT(ctrlParms.offset);
                int partition = FLASH_GET_PARTITION(ctrlParms.offset);
                unsigned long not_used;
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                int resumeWD;
#endif
              
                if(kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *)&not_used)==0)
                {
                    printk("\nERROR: Image does not support a NAND flash device.\n\n");
                    ret = -1;
                    break;
                }

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                resumeWD = bcm_suspend_watchdog();
#endif

                mutex_lock(&flashImageMutex);

                ret = flashFsKernelImage(ctrlParms.string, ctrlParms.strLen,
                    partition, &numPartitions);

                mutex_unlock(&flashImageMutex);

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                if (resumeWD)
                    bcm_resume_watchdog();
#endif

#if defined(GPL_CODE_DOWNGRADE_NVRAM_ADJUST)
                if(ret == 0 && (numPartitions == 1 || noReboot == 0))
		  {
                    if(AEI_IsDownGrade_FromSDK12To6(ctrlParms.string))
		      AEI_DownGrade_AdjustNVRAM();
                    resetPwrmgmtDdrMips();
		  }
#else
                if(ret == 0 && (numPartitions == 1 || noReboot == 0))
                    resetPwrmgmtDdrMips();
#endif
                }
                break;

            case BCM_IMAGE_KERNEL:  // not used for now.
                break;

            case BCM_IMAGE_WHOLE:
                {
                int noReboot = FLASH_IS_NO_REBOOT(ctrlParms.offset);
                int partition = FLASH_GET_PARTITION(ctrlParms.offset);
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                int resumeWD;
#endif

                if(ctrlParms.strLen <= 0)
                {
                    printk("Illegal flash image size [%d].\n", ctrlParms.strLen);
                    ret = -1;
                    break;
                }

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                resumeWD = bcm_suspend_watchdog();
#endif
                ret = commonImageWrite(FLASH_BASE, ctrlParms.string,
                    ctrlParms.strLen, &noReboot, partition );

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
                if (resumeWD)
                    bcm_resume_watchdog();
#endif
                if(ret == 0 && noReboot == 0)
                {
#ifdef GPL_CODE_CONFIG_JFFS
		    sys_sync();
#endif
                    resetPwrmgmtDdrMips();
                }
                else
                {
                    if (ret != 0)
                        printk("flash of whole image failed, ret=%d\n", ret);
                }
                }
                break;

            default:
                ret = -EINVAL;
                printk("flash_ioctl_command: invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_FLASH_READ:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                ret = kerSysScratchPadGet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
                break;

            case PERSISTENT:
                ret = kerSysPersistentGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case BACKUP_PSI:
                ret = kerSysBackupPsiGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#ifdef GPL_CODE_DEFAULT_CFG_CUSTOMER
            case CUSTOMER_PSI:
                ret = kerSysCustomerPsiGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
#endif
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
            case AEI_IMAGE1_PSI:
            
            case AEI_IMAGE2_PSI:
                 ret = AEI_kerSysImagePsiGet(ctrlParms.action,ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                 break;
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
            case OLD_IMAGE_PSI:
                ret = kerSysOldImageCfgActive();
                break;
#endif
            case SYSLOG:
                ret = kerSysSyslogGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case NVRAM:
                kerSysNvRamGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                ret = 0;
                break;

            case FLASH_SIZE:
                ret = kerSysFlashSizeGet();
                break;

            default:
                ret = -EINVAL;
                printk("Not supported.  invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_FLASH_LIST:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                ret = kerSysScratchPadList(ctrlParms.buf, ctrlParms.offset);
                break;

            default:
                ret = -EINVAL;
                printk("Not supported.  invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_DUMP_ADDR:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            dumpaddr( (unsigned char *) ctrlParms.string, ctrlParms.strLen );
            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_SET_MEMORY:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned long  *pul = (unsigned long *)  ctrlParms.string;
            unsigned short *pus = (unsigned short *) ctrlParms.string;
            unsigned char  *puc = (unsigned char *)  ctrlParms.string;
            switch( ctrlParms.strLen ) {
            case 4:
                *pul = (unsigned long) ctrlParms.offset;
                break;
            case 2:
                *pus = (unsigned short) ctrlParms.offset;
                break;
            case 1:
                *puc = (unsigned char) ctrlParms.offset;
                break;
            }
            /* This is placed as MoCA blocks are 32-bit only
            * accessible and following call makes access in terms
            * of bytes. Probably MoCA address range can be checked
            * here.
            */
            dumpaddr( (unsigned char *) ctrlParms.string, sizeof(long) );
            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_MIPS_SOFT_RESET:
        kerSysMipsSoftReset();
        break;

    case BOARD_IOCTL_LED_CTRL:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            kerSysLedCtrl((BOARD_LED_NAME)ctrlParms.strLen, (BOARD_LED_STATE)ctrlParms.offset);
            ret = 0;
        }
        break;

#if defined(GPL_CODE)
    case AEI_BOARD_IOCTL_LED_BRIGHTNESS_CTRL:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            AEI_kerSysLedBrightnessCtrl((BOARD_LED_NAME)ctrlParms.strLen, (unsigned int)ctrlParms.offset);
            ret = 0;
        }
        break;
#endif

    case BOARD_IOCTL_GET_ID:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,
            sizeof(ctrlParms)) == 0)
        {
            if( ctrlParms.string )
            {
                char p[NVRAM_BOARD_ID_STRING_LEN];
                kerSysNvRamGetBoardId(p);
#ifdef GPL_CODE
                if(strstr(p,"C1000"))
                {
                    memset(p,0,sizeof(p));
                    strcpy(p,"C1000A");
                }
                else if(strstr(p,"C1900A"))
                {
                    memset(p,0,sizeof(p));
                    strcpy(p,"C1900A");
                }
                else if(strstr(p,"C2000"))
                {
                    memset(p,0,sizeof(p));
                    strcpy(p,"C2000A");
                }
#endif
                if( strlen(p) + 1 < ctrlParms.strLen )
                    ctrlParms.strLen = strlen(p) + 1;
                __copy_to_user(ctrlParms.string, p, ctrlParms.strLen);
            }

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
        }
        break;
    
    case BOARD_IOCTL_GET_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ctrlParms.result = kerSysGetMacAddress( ucaMacAddr,
                ctrlParms.offset );

            if( ctrlParms.result == 0 )
            {
                __copy_to_user(ctrlParms.string, ucaMacAddr,
                    sizeof(ucaMacAddr));
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = ctrlParms.result;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_ALLOC_MAC_ADDRESSES:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ctrlParms.result = kerSysGetMacAddresses( ucaMacAddr,
                *((UINT32 *)ctrlParms.buf), ctrlParms.offset );

            if( ctrlParms.result == 0 )
            {
                __copy_to_user(ctrlParms.string, ucaMacAddr,
                    sizeof(ucaMacAddr));
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = ctrlParms.result;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_RELEASE_MAC_ADDRESSES:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if (copy_from_user((void*)ucaMacAddr, (void*)ctrlParms.string, \
                NVRAM_MAC_ADDRESS_LEN) == 0)
            {
                ctrlParms.result = kerSysReleaseMacAddresses( ucaMacAddr, *((UINT32 *)ctrlParms.buf) );
            }
            else
            {
                ctrlParms.result = -EACCES;
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = ctrlParms.result;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_RELEASE_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if (copy_from_user((void*)ucaMacAddr, (void*)ctrlParms.string, \
                NVRAM_MAC_ADDRESS_LEN) == 0)
            {
                ctrlParms.result = kerSysReleaseMacAddress( ucaMacAddr );
            }
            else
            {
                ctrlParms.result = -EACCES;
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = ctrlParms.result;
        }
        else
            ret = -EFAULT;
        break;

#if defined(GPL_CODE)
    case BOARD_IOCTL_GET_SN:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,
              sizeof(ctrlParms)) == 0)
        {
            if( ctrlParms.string )
            {
                char paramValue[32] = {0};

                aei_getNvramValueByParaName(AEI_NVPARAM_SN, paramValue, sizeof(paramValue));

                __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
            }

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        break;

    case BOARD_IOCTL_GET_HW_VERSION:
       if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
       {
           if( ctrlParms.string )
           {
               char paramValue[12] = {0};

               aei_getNvramValueByParaName(AEI_NVPARAM_HW_VERSION, paramValue, sizeof(paramValue));

               __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
           }

           ctrlParms.result = 0;
           __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
               sizeof(BOARD_IOCTL_PARMS));
       }
       break;

    case BOARD_IOCTL_GET_NVRAM_PARAM:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,
                sizeof(ctrlParms)) == 0)
        {
            if (ctrlParms.offset==AEI_NVPARAM_FW){
                if( ctrlParms.string )
                {
                    char paramValue[48] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_FW, paramValue, sizeof(paramValue));

                    __copy_to_user(ctrlParms.string, paramValue, 48 - 1);
                }
            }else if(ctrlParms.offset==AEI_NVPARAM_SN){
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_SN, paramValue, sizeof(paramValue));

                    __copy_to_user(ctrlParms.string, paramValue, 32 - 1);
                }

            }else if(ctrlParms.offset==AEI_NVPARAM_WPS_PIN){
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_WPS_PIN, paramValue, sizeof(paramValue));

                    __copy_to_user(ctrlParms.string, paramValue, 32 - 1);
                }

            }else if(ctrlParms.offset==AEI_NVPARAM_WPA_KEY){
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_WPA_KEY, paramValue, sizeof(paramValue));

                    __copy_to_user(ctrlParms.string, paramValue, 32 - 1);
                }

            }
#if defined(GPL_CODE)
            else if(ctrlParms.offset==AEI_NVPARAM_ADMIN_PW){
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_ADMIN_PW, paramValue, sizeof(paramValue));

                    __copy_to_user(ctrlParms.string, paramValue, 32 - 1);
                }
            }
#endif
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
            else if(ctrlParms.offset==AEI_NVPARAM_WLAN_FEATURE){
                char paramValue[NVRAM_WLAN_PARAMS_LEN]={0};

                aei_getNvramValueByParaName(AEI_NVPARAM_WLAN_FEATURE, paramValue, sizeof(paramValue));
                __copy_to_user(ctrlParms.string, paramValue, NVRAM_WLAN_PARAMS_LEN-1);
            }
#endif
#if defined(GPL_CODE_DETECT_BOARD_ID)
            else if(ctrlParms.offset == AEI_NVPARAM_DETECT_BID) {
                if( ctrlParms.string ) {
                    char data[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_DETECT_BID, data, sizeof(data));
                    __copy_to_user(ctrlParms.string, data, sizeof(data));
                }
            }
#endif
#ifdef GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG
            else if(ctrlParms.offset==AEI_NVPARAM_BOOTLINE)
            {
                if( ctrlParms.string )
                {
                    char *p;
                    char paramValue[NVRAM_BOOTLINE_LEN] = {0};

                    aei_getNvramValueByParaName(AEI_NVPARAM_BOOTLINE, paramValue, sizeof(paramValue));

                    for( p =  paramValue; p[2] != '\0'; p++ )
                    {
                        if( p[0] == 'p' && p[1] == '=' )
                        {
                            break;
                        }
                    }
                    __copy_to_user(ctrlParms.string, &p[2], 1);
                }

            }
#endif
#ifdef GPL_CODE_FACTORY_TEST
            else if (ctrlParms.offset == AEI_NVPARAM_WPA_KEY) {
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_WPA_KEY, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_WPS_PIN) {
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_WPS_PIN, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_ADMIN_PW) {
                if( ctrlParms.string )
                {
                    char paramValue[32] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_ADMIN_PW, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_BURN_IN_TEST) {
                if( ctrlParms.string ) {
                    char paramValue[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_BURN_IN_TEST, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_MANU_MODE) {
                if( ctrlParms.string ) {
                    char paramValue[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_MANU_MODE, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_ENABLE_BACKUP_PSI) {
                if( ctrlParms.string )
                {
                    char paramValue[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_ENABLE_BACKUP_PSI, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_SYSTEM_LOG_SIZE) {
                if( ctrlParms.string )
                {
                    char paramValue[5] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_SYSTEM_LOG_SIZE, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_AUXILLARY_FILE_SYSTEM_SIZE) {
                if( ctrlParms.string ) {
                    char paramValue[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_AUXILLARY_FILE_SYSTEM_SIZE, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_MAIN_THREAD_NUMBER) {
                if( ctrlParms.string ) {
                    char paramValue[5] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_MAIN_THREAD_NUMBER, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_NUMBER_MAC_ADDRESSES) {
                if( ctrlParms.string ) {
                    char paramValue[5] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_NUMBER_MAC_ADDRESSES, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_WLANFEATURE) {
                if( ctrlParms.string ) {
                    char paramValue[2] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_WLANFEATURE, paramValue, sizeof(paramValue));
                    __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_PSI_SIZE) {
                if( ctrlParms.string ) {
                    unsigned int psi_size=0;
                    char str_psi_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_PSI_SIZE, (char *)&psi_size, sizeof(psi_size));
                    snprintf(str_psi_size, sizeof(str_psi_size), "%d", psi_size);
                    __copy_to_user(ctrlParms.string, str_psi_size, sizeof(str_psi_size) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_VOICE_BOARD_ID) {
                if( ctrlParms.string ) {
                    char boardID[NVRAM_BOARD_ID_STRING_LEN] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_VOICE_BOARD_ID, (char *)&boardID, sizeof(boardID));
                    __copy_to_user(ctrlParms.string, boardID, sizeof(boardID));
                }
            }
            else if(ctrlParms.offset == AEI_NVPARAM_BASE_MAC_ADDRESS) {
                if( ctrlParms.string ) {
                    char mac[NVRAM_MAC_ADDRESS_LEN] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_BASE_MAC_ADDRESS, mac, sizeof(mac));
                    __copy_to_user(ctrlParms.string, mac, sizeof(mac));
                }
            }
            else if(ctrlParms.offset == AEI_NVPARAM_FACTORY_FW_VERSION) {
                if( ctrlParms.string ) {
                    char FW_version[48] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_FACTORY_FW_VERSION, FW_version, sizeof(FW_version));
                    __copy_to_user(ctrlParms.string, FW_version, sizeof(FW_version));
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_RDP_TM_SIZE) {
                if( ctrlParms.string ) {
                    unsigned char size=0;
                    char str_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_RDP_TM_SIZE, (char *)&size, sizeof(size));
                    snprintf(str_size, sizeof(str_size), "%u", size);
                    __copy_to_user(ctrlParms.string, str_size, sizeof(str_size) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_RDP_MC_SIZE) {
                if( ctrlParms.string ) {
                    unsigned char size=0;
                    char str_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_RDP_MC_SIZE, (char *)&size, sizeof(size));
                    snprintf(str_size, sizeof(str_size), "%u", size);
                    __copy_to_user(ctrlParms.string, str_size, sizeof(str_size) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_DHD_MEM_0_ZISE) {
                if( ctrlParms.string ) {
                    unsigned char size=0;
                    char str_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_DHD_MEM_0_ZISE, (char *)&size, sizeof(size));
                    snprintf(str_size, sizeof(str_size), "%u", size);
                    __copy_to_user(ctrlParms.string, str_size, sizeof(str_size) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_DHD_MEM_1_ZISE) {
                if( ctrlParms.string ) {
                    unsigned char size=0;
                    char str_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_DHD_MEM_1_ZISE, (char *)&size, sizeof(size));
                    snprintf(str_size, sizeof(str_size), "%u", size);
                    __copy_to_user(ctrlParms.string, str_size, sizeof(str_size) - 1);
                }
            }
            else if (ctrlParms.offset == AEI_NVPARAM_DHD_MEM_2_ZISE) {
                if( ctrlParms.string ) {
                    unsigned char size=0;
                    char str_size[10] = {0};
                    aei_getNvramValueByParaName(AEI_NVPARAM_DHD_MEM_2_ZISE, (char *)&size, sizeof(size));
                    snprintf(str_size, sizeof(str_size), "%u", size);
                    __copy_to_user(ctrlParms.string, str_size, sizeof(str_size) - 1);
                }
            }
#endif

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
        }
        break;

    case BOARD_IOCTL_RESET_NVRAM_PARAM:
       if (copy_from_user((void*)&ctrlParms, (void*)arg,
             sizeof(ctrlParms)) == 0)
       {
           NVRAM_DATA nvramData;
           AEI_readNvramData(&nvramData);
           if (ctrlParms.offset==AEI_NVPARAM_FW){
               memset(&nvramData.chFactoryFWVersion[0],0,48);
               nvramData.chFactoryFWVersion[0]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }else if(ctrlParms.offset==AEI_NVPARAM_SN){
               memset(&nvramData.ulSerialNumber[0],0,32);
               nvramData.ulSerialNumber[0]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }else if(ctrlParms.offset==AEI_NVPARAM_WPS_PIN){
               memset(&nvramData.wpsPin[0],0,32);
               nvramData.wpsPin[0]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }else if(ctrlParms.offset==AEI_NVPARAM_WPA_KEY){
               memset(&nvramData.wpaKey[0],0,32);
               nvramData.wpaKey[0]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }
#if defined(GPL_CODE)
           else if(ctrlParms.offset==AEI_NVPARAM_ADMIN_PW){
               memset(&nvramData.adminPassword[0],0,32);
               nvramData.adminPassword[0]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }
#endif
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
           else if(ctrlParms.offset==AEI_NVPARAM_WLAN_FEATURE){
               nvramData.wlanParams[NVRAM_WLAN_PARAMS_LEN -1]='\0';
               writeNvramDataCrcLocked(&nvramData);
           }
#endif
           ctrlParms.result = 0;
           __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));

       }
       break;
    case BOARD_IOCTL_SET_NVRAM_PARAM:
       if (copy_from_user((void*)&ctrlParms, (void*)arg,
            sizeof(ctrlParms)) == 0)
       {
           if(ctrlParms.string && strlen(ctrlParms.string)>0)
           {
               NVRAM_DATA nvramData;
               AEI_readNvramData(&nvramData);
               if (ctrlParms.offset==AEI_NVPARAM_FW){
                   memset(&nvramData.chFactoryFWVersion[0],0,48);
                   strncpy(&nvramData.chFactoryFWVersion[0],ctrlParms.string,48-1);
                   writeNvramDataCrcLocked(&nvramData);
               }else if(ctrlParms.offset==AEI_NVPARAM_SN){
                   memset(&nvramData.ulSerialNumber[0],0,32);
                   strncpy(&nvramData.ulSerialNumber[0],ctrlParms.string,32-1);
                   writeNvramDataCrcLocked(&nvramData);
               }else if(ctrlParms.offset==AEI_NVPARAM_WPS_PIN){
                   memset(&nvramData.wpsPin[0],0,32);
                   strncpy(&nvramData.wpsPin[0],ctrlParms.string,32-1);
                   writeNvramDataCrcLocked(&nvramData);
               }else if(ctrlParms.offset==AEI_NVPARAM_WPA_KEY){
                   memset(&nvramData.wpaKey[0],0,32);
                   strncpy(&nvramData.wpaKey[0],ctrlParms.string,32-1);
                   writeNvramDataCrcLocked(&nvramData);
               }
#if defined(GPL_CODE)
               else if(ctrlParms.offset==AEI_NVPARAM_ADMIN_PW){
                   memset(&nvramData.adminPassword[0],0,32);
                   strncpy(&nvramData.adminPassword[0],ctrlParms.string,32-1);
                   writeNvramDataCrcLocked(&nvramData);
               }
#endif
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
               else if(ctrlParms.offset==AEI_NVPARAM_WLAN_FEATURE){
                   int wlan_feature=0;
                   if(!strncmp(ctrlParms.string,"0x",2))
                   {
                       if(kstrtoint(ctrlParms.string, 16, &wlan_feature))
                       {
                           ret = -EINVAL;
                       }
                   }
                   else
                   {
                       if(kstrtoint(ctrlParms.string, 10, &wlan_feature))
                       {
                           ret = -EINVAL;
                       }
                   }
                   if(ret != -EINVAL)
                   {
                       nvramData.wlanParams[NVRAM_WLAN_PARAMS_LEN -1]= (unsigned char)wlan_feature;
                       writeNvramDataCrcLocked(&nvramData);
                   }
               }
#endif

           }
           ctrlParms.result = 0;
           __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
               sizeof(BOARD_IOCTL_PARMS));
        }
        break;

#if defined(GPL_CODE_CHECK)
    /*the nvram adjustment for SDK3->SDK6->SDK12 is no longer used for SDK16, just keep the defines here*/
    case BOARD_IOCTL_ADJUST_NVRAM:
        break;

#endif
#endif   //GPL_CODE
#if defined(GPL_CODE_DEBUG_NVRAM)
         case BOARD_IOCTL_PRINT_NVRAM:
			//if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
				NVRAM_DATA nvramData;
				AEI_readNvramData(&nvramData);
				printk("@@@@@@@@@@NVRAM ulversion:		 %d\n", nvramData.ulVersion);
				printk("@@@@@@@@@@NVRAM szBootline:		 %s\n", nvramData.szBootline);
				printk("@@@@@@@@@@NVRAM szBoardId:		 %s\n", nvramData.szBoardId);
				printk("@@@@@@@@@@NVRAM szBoardId:		 %d\n", nvramData.ulPsiSize);
				printk("@@@@@@@@@@NVRAM ulNumMacAddrs:		 %d\n", nvramData.ulNumMacAddrs);
				printk("@@@@@@@@@@NVRAM ucaBaseMacAddr:		 %s\n", nvramData.ucaBaseMacAddr);
				printk("@@@@@@@@@@NVRAM ulSerialNumber:		 %s\n", nvramData.ulSerialNumber);
				printk("@@@@@@@@@@NVRAM chFactoryFWVersion:		 %s\n", nvramData.chFactoryFWVersion);
				printk("@@@@@@@@@@NVRAM wpsPin:		 %s\n", nvramData.wpsPin);
				printk("@@@@@@@@@@NVRAM wpaKey:		 %s\n\n", nvramData.wpaKey);
				ctrlParms.result = 0;				
             }
             break;
#endif
#if defined(GPL_CODE_UPGRADE_DUALIMG_HISTORY_SPAD) || defined(GPL_CODE_CUSTOMER_REVERT_FIRMWARE_CONFIG)
    case BOARD_IOCTL_GET_DUAL_FW_VERSION:
       if (copy_from_user((void*)&ctrlParms, (void*)arg,
            sizeof(ctrlParms)) == 0)
       {

           if (ctrlParms.offset==0){ //image 0
               if( ctrlParms.string )
               {
                   PFILE_TAG pTag1 = getTagFromPartition(1);
                   if(pTag1)
                   {
#if defined(GPL_CODE_TWO_IN_ONE_FIRMWARE)
                       unsigned char boardid[16]={0};
#endif
#if defined(GPL_CODE_TWO_IN_ONE_FIRMWARE) && defined(GPL_CODE)
                       if((kerSysGetBoardID(boardid)==0) && (!strcmp(boardid,"C2000A") || !strcmp(boardid,"C1900A")))
                       {
                           if (!strcmp(boardid,"C2000A"))
                               __copy_to_user(ctrlParms.string, pTag1->imageVersion, SIG_LEN_2 - 1);
                           else if (!strcmp(boardid,"C1900A"))
                               __copy_to_user(ctrlParms.string, pTag1->imageVersion_2, SIG_LEN_2 - 1);
                       }
                       else
#endif
                       __copy_to_user(ctrlParms.string, pTag1->signiture_2, SIG_LEN_2 - 1);
                   }
                   else
                       __copy_to_user(ctrlParms.string, "", SIG_LEN_2 - 1);
                }
            }else if(ctrlParms.offset==1){ //image 1
                if( ctrlParms.string )
                {
                    PFILE_TAG pTag2 = getTagFromPartition(2);
                    if(pTag2)
                    {
#if defined(GPL_CODE_TWO_IN_ONE_FIRMWARE)
                        unsigned char boardid[16]={0};
#endif
#if defined(GPL_CODE_TWO_IN_ONE_FIRMWARE) && defined(GPL_CODE)
                        if((kerSysGetBoardID(boardid)==0) && (!strcmp(boardid,"C2000A") || !strcmp(boardid,"C1900A")))
                        {
                            if (!strcmp(boardid,"C2000A"))
                                __copy_to_user(ctrlParms.string, pTag2->imageVersion, SIG_LEN_2 - 1);
                            else if (!strcmp(boardid,"C1900A"))
                                __copy_to_user(ctrlParms.string, pTag2->imageVersion_2, SIG_LEN_2 - 1);
                        }
                        else
#endif
                        __copy_to_user(ctrlParms.string, pTag2->signiture_2, SIG_LEN_2 - 1);
                    }
                    else
                        __copy_to_user(ctrlParms.string, "", SIG_LEN_2 - 1);

                }

            }

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                 sizeof(BOARD_IOCTL_PARMS));
        }
        break;
#endif
  case  BOARD_IOCTL_SET_UG_INFO:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if(ctrlParms.string && strlen(ctrlParms.string)>0)
            {
                if (ctrlParms.offset==0)
                {
                    char ug_info[2]={0};
                    strncpy(ug_info,ctrlParms.string,1);
                    kerSysScratchPadSet("UpGrade_Info",ug_info,sizeof(ug_info));
                 }
             }
             ctrlParms.result = 0;
             __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        break;
    case  BOARD_IOCTL_GET_UG_INFO:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,sizeof(ctrlParms)) == 0)
        {
            if (ctrlParms.offset==0)
            {
                if( ctrlParms.string )
                {
                     char ug_info[2]={0};
                     kerSysScratchPadGet("UpGrade_Info",ug_info,sizeof(ug_info));
                     __copy_to_user(ctrlParms.string, ug_info,1);
                }
            }
            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        break;

#if defined(GPL_CODE)
    case BOARD_IOCTL_GET_FS_OFFSET:
        {
#if defined(GPL_CODE_CONFIG_JFFS)
            struct mtd_info *mtd1 = get_mtd_device_nm("nvram");
            if( mtd1 )
            {
                ctrlParms.result = mtd1->erasesize;
            }
            else
            {
                ctrlParms.result = 0;
            }
#else
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            ctrlParms.result = fInfo.flash_rootfs_start_offset;
#endif
            //printk("###offset(%x)\n",ctrlParms.result);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;
#endif

    case BOARD_IOCTL_GET_PSI_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            ctrlParms.result = fInfo.flash_persistent_length;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_BACKUP_PSI_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            // if number_blks > 0, that means there is a backup psi, but length is the same
            // as the primary psi (persistent).

            ctrlParms.result = (fInfo.flash_backup_psi_number_blk > 0) ?
                fInfo.flash_persistent_length : 0;
            printk("backup_psi_number_blk=%d result=%d\n", fInfo.flash_backup_psi_number_blk, fInfo.flash_persistent_length);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_SYSLOG_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            ctrlParms.result = fInfo.flash_syslog_length;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_SDRAM_SIZE:
        ctrlParms.result = (int) g_ulSdramSize;
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
        break;

    case BOARD_IOCTL_GET_BASE_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            __copy_to_user(ctrlParms.string, g_pMacInfo->ucaBaseMacAddr, NVRAM_MAC_ADDRESS_LEN);
            ctrlParms.result = 0;

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_GET_CHIP_ID:
#if defined(GPL_CODE_CHIP_D0_CHECK)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if(ctrlParms.offset==1 && ctrlParms.string)
            {
#if defined(GPL_CODE_CHIP_D0_CHECK)
	//This is a special requirement from PLM. Please don't override it.
                sprintf(ctrlParms.string, "%x", (int)CPURevId);
#else
                sprintf(ctrlParms.string, "%x", (int)(PERF->RevID));
#endif
            }
        }  	
#endif
        ctrlParms.result = kerSysGetChipId();


        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
        break;

    case BOARD_IOCTL_GET_CHIP_REV:
        ctrlParms.result = UtilGetChipRev();
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
        break;

#if defined(GPL_CODE)
    case BOARD_IOCTL_GET_FLASH_TOTAL:
#if defined(GPL_CODE_63168_CHIP)
         /*ARM platform will be crashed by following nand api*/
         ctrlParms.result = AEI_nand_flash_get_total_size()/1024;
#else
         ctrlParms.result = flash_get_total_size()/1024;
#endif
         __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
         ret = 0;
        break;
#if defined(GPL_CODE_63168_CHIP)
    case AEI_BOARD_IOCTL_GET_FLASH_DEVICE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,
              sizeof(ctrlParms)) == 0)
        {
            if( ctrlParms.string )
            {
                char paramValue[32] = {0};

                AEI_nand_flash_get_device_name(paramValue);
                
                __copy_to_user(ctrlParms.string, paramValue, sizeof(paramValue) - 1);
            }

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        break;

#endif
    case BOARD_IOCTL_GET_POWERLED_STATUS:
         ctrlParms.result = gPowerLedStatus;
         __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
         ret = 0;
        break;
#endif

    case BOARD_IOCTL_GET_NUM_ENET_MACS:
    case BOARD_IOCTL_GET_NUM_ENET_PORTS:
        {
            const ETHERNET_MAC_INFO *EnetInfos;
            int i, cnt, numEthPorts = 0;
            if ( ( EnetInfos = BpGetEthernetMacInfoArrayPtr() ) != NULL ) {
                for( i = 0; i < BP_MAX_ENET_MACS; i++) {
                    if (EnetInfos[i].ucPhyType != BP_ENET_NO_PHY) {
                        bitcount(cnt, EnetInfos[i].sw.port_map);
                        numEthPorts += cnt;
                    }
                }
                ctrlParms.result = numEthPorts;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_CFE_VER:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned char vertag[CFE_VERSION_MARK_SIZE+CFE_VERSION_SIZE];
            kerSysCfeVersionGet(vertag, sizeof(vertag));
            if (ctrlParms.strLen < CFE_VERSION_SIZE) {
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = -EFAULT;
            }
            else if (strncmp(vertag, "cfe-v", 5)) { // no tag info in flash
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ctrlParms.result = 1;
                __copy_to_user(ctrlParms.string, vertag+CFE_VERSION_MARK_SIZE, CFE_VERSION_SIZE);
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
        }
        else {
            ret = -EFAULT;
        }
        break;

#if defined (WIRELESS)
    case BOARD_IOCTL_GET_WLAN_ANT_INUSE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned short antInUse = 0;
            if (BpGetWirelessAntInUse(&antInUse) == BP_SUCCESS) {
                if (ctrlParms.strLen == sizeof(antInUse)) {
                    __copy_to_user(ctrlParms.string, &antInUse, sizeof(antInUse));
                    ctrlParms.result = 0;
                    __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                    ret = 0;
                } else
                    ret = -EFAULT;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif
    case BOARD_IOCTL_SET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
            ctrlParms.result = -EFAULT;
            ret = -EFAULT;
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                board_ioc->eventmask |= *((int*)ctrlParms.string);
#if defined (WIRELESS)
                if((board_ioc->eventmask & SES_EVENTS)) {
                    ctrlParms.result = 0;
                    ret = 0;
                }
#endif
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            }
            break;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_GET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                __copy_to_user(ctrlParms.string, &board_ioc->eventmask, sizeof(unsigned long));
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_UNSET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
                board_ioc->eventmask &= (~(*((int*)ctrlParms.string)));
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#if defined (WIRELESS)
    case BOARD_IOCTL_SET_SES_LED:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            if (ctrlParms.strLen == sizeof(int)) {
                sesLed_ctrl(*(int*)ctrlParms.string);
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif

    case BOARD_IOCTL_GET_GPIOVERLAYS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
	    unsigned long GPIOOverlays = 0;
	    ret = 0;
            if (BP_SUCCESS == (ctrlParms.result = BpGetGPIOverlays(&GPIOOverlays) )) {
	        __copy_to_user(ctrlParms.string, &GPIOOverlays, sizeof(unsigned long));

	        if(__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS))!=0)
		    ret = -EFAULT;	    		  
	    }
	}else
            ret = -EFAULT;

        break;
#if defined(GPL_CODE_VOIP_LED)
     case AEI_BOARD_IOCTL_VOIP_LED:
#if defined(GPL_CODE_63168_CHIP)
         if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
             if (ctrlParms.strLen == 8) {
                 AEI_VoipLed_ctrl((char *)ctrlParms.string);
                 ctrlParms.result = 0;
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                 ret = 0;
             } else
                 ret = -EFAULT;

             break;
         }
         else {
             ret = -EFAULT;
         }
#endif
         break;
#endif
#if defined(GPL_CODE)
     case AEI_BOARD_IOCTL_WIRELESS_REDLED:
#if defined(GPL_CODE_63168_CHIP)
         if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
             if (ctrlParms.strLen == sizeof(int)) {
                 AEI_wlanLed_ctrl(*(int*)ctrlParms.string);
                 ctrlParms.result = 0;
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                 ret = 0;
             } else
                 ret = -EFAULT;

             break;
         }
         else {
             ret = -EFAULT;
         }
#endif
         break;
    case BOARD_IOCTL_SET_WLANLEDMODE:
#if defined(GPL_CODE_63168_CHIP)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            /*change wireless led mode */
            if (ctrlParms.strLen == sizeof(int)) {
                if ( *(int*)ctrlParms.string == 1) {
                    GPIO->GPIOCtrl &= ~(0x3 << 4); /*wlan control*/
                }
                else
                    GPIO->GPIOCtrl |= (0x3 << 4); /*periph control*/

                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }else
                ret = -EFAULT;
            break;
        }else {
            ret = -EFAULT;
        }
#endif
        break;
    case BOARD_IOCTL_GET_WLANLEDMODE:
#if defined(GPL_CODE_63168_CHIP)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            int ledctrlmode = 0;
            int temp = 0;
            /*Get wireless led mode */
            if (ctrlParms.strLen == sizeof(int)) {
                printk("%x\n",GPIO->GPIOCtrl);
                temp = GPIO->GPIOCtrl & (0x3 << 4);
                if ( temp == 0x0){
                    ledctrlmode = 1;/*wlan control*/
                    printk("ledctrlmode=wlan control\n");
                }
                else {
                    ledctrlmode = 0;/*periph control*/
                    printk("ledctrlmode=periph control\n");
                }

                __copy_to_user(ctrlParms.string,(char *)&ledctrlmode,sizeof(int));

                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }else
                ret = -EFAULT;
            break;
        }else {
            ret = -EFAULT;
        }
#endif /*AEI_63168_CHIP*/
        break;
#endif /*end GPL_CODE*/

    case BOARD_IOCTL_SET_MONITOR_FD:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {

           g_monitor_nl_pid =  ctrlParms.offset;
           printk(KERN_INFO "monitor task is initialized pid= %d \n",g_monitor_nl_pid);
        }
        break;

    case BOARD_IOCTL_WAKEUP_MONITOR_TASK:
        kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_WAKEUP_MONITOR_TASK, NULL, 0);
        break;

    case BOARD_IOCTL_SET_CS_PAR:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            ret = ConfigCs(&ctrlParms);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SET_GPIO:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            kerSysSetGpioState(ctrlParms.strLen, ctrlParms.offset);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;

#if defined(CONFIG_BCM_CPLD1)
    case BOARD_IOCTL_SET_SHUTDOWN_MODE:
        BcmCpld1SetShutdownMode();
        ret = 0;
        break;

    case BOARD_IOCTL_SET_STANDBY_TIMER:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BcmCpld1SetStandbyTimer(ctrlParms.offset);
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif

    case BOARD_IOCTL_BOOT_IMAGE_OPERATION:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch(ctrlParms.offset)
            {
            case BOOT_SET_PART1_IMAGE:
            case BOOT_SET_PART2_IMAGE:
            case BOOT_SET_PART1_IMAGE_ONCE:
            case BOOT_SET_PART2_IMAGE_ONCE:
            case BOOT_SET_OLD_IMAGE:
            case BOOT_SET_NEW_IMAGE:
            case BOOT_SET_NEW_IMAGE_ONCE:
                ctrlParms.result = kerSysSetBootImageState(ctrlParms.offset);
                break;

            case BOOT_GET_BOOT_IMAGE_STATE:
#if GPL_CODE
                ctrlParms.result = getBootedValue(1) == BOOTED_PART1_IMAGE ? 1 : 2;
#else
                ctrlParms.result = kerSysGetBootImageState();
#endif
                break;

            case BOOT_GET_IMAGE_VERSION:
                /* ctrlParms.action is parition number */
                ctrlParms.result = getImageVersion((int) ctrlParms.action,
                    ctrlParms.string, ctrlParms.strLen);
                break;

            case BOOT_GET_BOOTED_IMAGE_ID:
                /* ctrlParm.strLen == 1: partition or == 0: id (new or old) */
                ctrlParms.result = getBootedValue(ctrlParms.strLen);
                break;

            default:
                ctrlParms.result = -EFAULT;
                break;
            }
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_GET_TIMEMS:
        ret = jiffies_to_msecs(jiffies - INITIAL_JIFFIES);
        break;

    case BOARD_IOCTL_GET_DEFAULT_OPTICAL_PARAMS:
    {
        unsigned char ucDefaultOpticalParams[BP_OPTICAL_PARAMS_LEN];
            
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ret = 0;
            if (BP_SUCCESS == (ctrlParms.result = BpGetDefaultOpticalParams(ucDefaultOpticalParams)))
            {
                __copy_to_user(ctrlParms.string, ucDefaultOpticalParams, BP_OPTICAL_PARAMS_LEN);

                if (__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS)) != 0)
                {
                    ret = -EFAULT;
                }
            }                        
        }
        else
        {
            ret = -EFAULT;
        }

        break;
    }
    
    break;
    case BOARD_IOCTL_GET_GPON_OPTICS_TYPE:
     
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
        {
            unsigned short Temp=0;
            BpGetGponOpticsType(&Temp);
            *((UINT32*)ctrlParms.buf) = Temp;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }        
        ret = 0;

        break;

#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM_6802_MoCA) || defined(BRCM_XDSL_DISTPOINT)
    case BOARD_IOCTL_SPI_SLAVE_INIT:  
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device */
             if (kerSysBcmSpiSlaveInit(ctrlParms.result) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }        
        }
        else
        {
            ret = -EFAULT;
        }        
        break;   
        
    case BOARD_IOCTL_SPI_SLAVE_READ:  
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device for reads */
             if (kerSysBcmSpiSlaveRead(ctrlParms.result, ctrlParms.offset, (unsigned long *)ctrlParms.buf, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             } 
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));    
             }
        }
        else
        {
            ret = -EFAULT;
        }                 
        break;    
        
    case BOARD_IOCTL_SPI_SLAVE_WRITE_BUF:
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device for write buf */
             if (kerSysBcmSpiSlaveWriteBuf(ctrlParms.result, ctrlParms.offset, (unsigned long *)ctrlParms.buf, ctrlParms.strLen, 4) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SPI_SLAVE_WRITE:  
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for writes */
             if (kerSysBcmSpiSlaveWrite((uint32_t)ctrlParms.buf, ctrlParms.offset, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             } 
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));    
             }
        }
        else
        {
            ret = -EFAULT;
        }                 
        break;
    case BOARD_IOCTL_SPI_SLAVE_SET_BITS:
        ret = 0;
#if defined(CONFIG_BCM_6802_MoCA)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for set bits */
             if (kerSysBcmSpiSlaveModify((uint32_t)ctrlParms.buf, ctrlParms.offset, ctrlParms.result, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             } 
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));    
             }
        }
        else
        {
            ret = -EFAULT;
        }                 
#endif
        break;
    case BOARD_IOCTL_SPI_SLAVE_CLEAR_BITS:
        ret = 0;
#if defined(CONFIG_BCM_6802_MoCA)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for clear bits */
             if (kerSysBcmSpiSlaveModify((uint32_t)ctrlParms.buf, ctrlParms.offset, 0, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             } 
             else
             {
                   __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));    
             }
        }
        else
        {
            ret = -EFAULT;
        }                 
#endif
        break;    
#endif

#if defined(CONFIG_EPON_SDK)
     case BOARD_IOCTL_GET_PORT_MAC_TYPE:
        {
            unsigned short port;
            unsigned long mac_type;

            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
                port = ctrlParms.offset;
                if (BpGetPortMacType(port, &mac_type) == BP_SUCCESS) {
                    ctrlParms.result = (unsigned int)mac_type; 
                    __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                    ret = 0;
                }
                else {
                    ret = -EFAULT;
                }
                break;
            }
        }

    case BOARD_IOCTL_GET_NUM_FE_PORTS:
        {
            unsigned long fe_ports;
            if (BpGetNumFePorts(&fe_ports) == BP_SUCCESS) {
                ctrlParms.result = fe_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_NUM_GE_PORTS:
        {
            unsigned long ge_ports;
            if (BpGetNumGePorts(&ge_ports) == BP_SUCCESS) {
                ctrlParms.result = ge_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_NUM_VOIP_PORTS:
        {
            unsigned long voip_ports;
            if (BpGetNumVoipPorts(&voip_ports) == BP_SUCCESS) {
                ctrlParms.result = voip_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_SWITCH_PORT_MAP:
        {
            unsigned long port_map;
            if (BpGetSwitchPortMap(&port_map) == BP_SUCCESS) {
                ctrlParms.result = port_map;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }


    case BOARD_IOCTL_GET_EPON_GPIOS:
        {
        	int i, rc = 0, gpionum;
        	unsigned short *pusGpio, gpio;
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
                if( ctrlParms.string )
                {
                    /* walk through the epon gpio list */
                	i = 0;
                	pusGpio = (unsigned short *)ctrlParms.string;
                	gpionum =  ctrlParms.strLen/sizeof(unsigned short);
                    for(;;)
                    {
                     	rc = BpGetEponGpio(i, &gpio);
                       	if( rc == BP_MAX_ITEM_EXCEEDED || i >= gpionum )
                       		break;
                       	else
                       	{
                       		if( rc == BP_SUCCESS )
                       			*pusGpio = gpio;
                       		else
                       			*pusGpio = BP_GPIO_NONE;
                       		pusGpio++;
                       	}
                       	i++;
                     }
                     ctrlParms.result = 0;
                     __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                }
                else
                {
                    ret = -EFAULT;
                }
            }
            break;
        }
#endif


#if defined(GPL_CODE_SMARTLED)
    case BOARD_IOCTL_SET_INET_TRAFFIC_BLINK:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            register uint32 *pv32;

#ifdef GPL_CODE_6368_CHIP //For 6368
            pv32 = (uint32 *)0xb0000090;
            if (ctrlParms.strLen == 0)
               (*pv32) |= 0x00004000;
            else
               (*pv32) &= 0xffffbfff;
#if defined(GPL_CODE)
            /*Activity blink of .5 seconds on and off during LAN to WAN and WAN to LAN activity.*/
            pv32 = (uint32 *)0xb00018ac;
            //printk("18ac =%x\n",*pv32);
            (*pv32) |= 0x00000060; // 0x60 = 01100000
#endif
#endif
            ctrlParms.result = 1;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif /* AEI_VDSL_SMARTLED */
#if defined(GPL_CODE_CHECK_FLASH_ID)
    case AEI_BOARD_IOCTL_GET_NAND_FLASH_ID:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            int Temp=0;
            Temp=AEI_get_flash_mafId();
            *((UINT32*)ctrlParms.buf) = Temp;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        ret = 0;
#endif
    case BOARD_IOCTL_GET_BATTERY_ENABLE:
        ret = kerSysIsBatteryEnabled();
        break;

    case BOARD_IOCTL_MEM_ACCESS:
        {
            BOARD_MEMACCESS_IOCTL_PARMS parms;
            void *va;
            int i;
            int j;
            int blen;
            unsigned char *cp,*bcp;
            unsigned short *sp,*bsp;
            unsigned long *lp,*blp;
            if (copy_from_user((void*)&parms, (void*)arg, sizeof(parms))) 
            {
                ret = -EFAULT;
                break;
            }
            if (parms.op == BOARD_MEMACCESS_IOCTL_OP_FILL) {
                blen = parms.size;
            } else {
                blen = parms.size * parms.count;
            }
            bcp = (unsigned char *)kmalloc(blen, GFP_KERNEL);
            bsp = (unsigned short *)bcp;
            blp = (unsigned long *)bcp;
            if (copy_from_user((void*)bcp, (void*)parms.buf, blen)) 
            {
                ret = -EFAULT;
                kfree(bcp);
                break;
            }
            switch(parms.space) {
            case BOARD_MEMACCESS_IOCTL_SPACE_REG:
                va = ioremap_nocache((int)parms.address, blen);
                break;
            case BOARD_MEMACCESS_IOCTL_SPACE_KERN:
                va = parms.address;
                break;
            default:
                va = NULL;
                ret = -EFAULT;
            }
            // printk("memacecssioctl address started %08x mapped to %08x size is %d count is %d\n",(int)parms.address, (int)va,parms.size, parms.count);
            cp = (unsigned char *)va;
            sp = (unsigned short *)((int)va & ~1);
            lp = (unsigned long *)((int)va & ~3);
            for (i=0; i < parms.count; i++) {
                if ((parms.op == BOARD_MEMACCESS_IOCTL_OP_WRITE) 
                   || (parms.op == BOARD_MEMACCESS_IOCTL_OP_FILL)) {
                    j = 0;
                    if (parms.op == BOARD_MEMACCESS_IOCTL_OP_WRITE) 
                    {
                        j = i;
                    }
                    switch(parms.size) {
                    case 1:
                        cp[i] = bcp[j];
                        break;
                    case 2:
                        sp[i] = bsp[j];
                        break;
                    case 4:
                        lp[i] = blp[j];
                        break;
                    }
                } else {
                    switch(parms.size) {
                    case 1:
                        bcp[i] = cp[i];
                        break;
                    case 2:
                        bsp[i] = sp[i];
                        break;
                    case 4:
                        blp[i] = lp[i];
                        break;
                    }
                }
            }
            __copy_to_user((void *)parms.buf, (void*)bcp, blen);
            if (va != parms.address)
            {
                iounmap(va);
            }
            kfree(bcp);
        }
        break;

#if !defined(CONFIG_BCM960333)
    case BOARD_IOCTL_SET_DYING_GASP_INTERRUPT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
           if (ctrlParms.offset)
              kerSysEnableDyingGaspInterrupt();
           else
              kerSysDisableDyingGaspInterrupt();
        }
        break;
#endif

#if defined(GPL_CODE_FACTORY_TEST)
    case AEI_BOARD_IOCTL_SET_RMA_STATUS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
        {
            g_RMA_status = ctrlParms.offset - '0';
	    ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif
#if defined(GPL_CODE)
    case AEI_BOARD_IOCTL_SET_WAN_TYPE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            aeiWanType = ctrlParms.offset;
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif
#if defined(GPL_CODE_FACTORY_TELNET)
    case AEI_BOARD_IOCTL_SET_RESTOREDEFAULT_FLAG:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if(ctrlParms.string && ctrlParms.strLen ==1)
            {
                if (ctrlParms.offset==0)
                {
                    ret=kerSysScratchPadSet("RestoreDefault",ctrlParms.string,ctrlParms.strLen);
                }
                else{
                    ret = -EFAULT;
                }
            }
            else{
                ret = -EFAULT;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
       }
       else {
            ret = -EFAULT;
       }

       break;
    case AEI_BOARD_IOCTL_GET_RESTOREDEFAULT_FLAG:
	if (copy_from_user((void*)&ctrlParms, (void*)arg,sizeof(ctrlParms)) == 0)
        {
             if (ctrlParms.offset==0)
	     {
                if( ctrlParms.string )
                {
                  ret= kerSysScratchPadGet("RestoreDefault",ctrlParms.string,ctrlParms.strLen);
                }
                else{
                    ret = -EFAULT;
                }
             }
             else{
                 ret = -EFAULT;
             }
             ctrlParms.result = ret;
             __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else {
            ret = -EFAULT;
        }

	break;
#endif

#ifdef GPL_CODE_NAND_CNT_128K
	case AEI_BOARD_IOCTL_GET_FLASH_CNT:
		
		if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
		{

            switch (ctrlParms.action) 
            {
            	case AEI_GET_FLASH_CNT_BUF_SIZE:
            		ret = AEI_kerSysGetFlashCNTBufSize(ctrlParms.string,ctrlParms.buf);
                break;
            
                case AEI_GET_FLASH_CNT:
                	ret = AEI_kerSysGetFlashCNT(ctrlParms.string,ctrlParms.buf);
                break;
                     
                default:
                	ret = -1;
                break;
                
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
         }
	break;
#endif

#if defined(GPL_CODE)
	case AEI_BOARD_IOCTL_SET_NVRAM_DATA:
		if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
		{
            switch (ctrlParms.offset) 
            {
            	case NVRAM_SET_BREAK_INTO_CFE:
                {       
                    NVRAM_DATA nvramData;
                    AEI_readNvramData(&nvramData);

                    if ( nvramData.breakIntoCfe != (unsigned char)(ctrlParms.action) )
                    {
                        printk("nvram data action is changed from %d to  %d\n", nvramData.breakIntoCfe, ctrlParms.action);
                        nvramData.breakIntoCfe = (unsigned char)(ctrlParms.action);
                        writeNvramDataCrcLocked(&nvramData);
                    }
                    else
                        printk("nvram data is not changed, needn't update nvram\n");
                }
                break;

                default:
                	ret = -1;
                break;
            }
             ctrlParms.result = 0;
             __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
         }
	break;
#endif


#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96848)
    case BOARD_IOCTL_GET_BOOT_SECURE:
        ctrlParms.result = otp_is_boot_secure();
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(ctrlParms));
        ret = 0;
        break;

    case BOARD_IOCTL_GET_BTRM_BOOT:
        ctrlParms.result = otp_is_btrm_boot();
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(ctrlParms));
        ret = 0;
        break;
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    case BOARD_IOCTL_SATA_TEST:
        if( bcm_sata_test_ioctl_fn)
        {
            ret = bcm_sata_test_ioctl_fn((void*)arg);
        }
        else
        {
            printk("SATA TEST module not loaded\n");
        }
        break;
#endif
    default:
        ret = -EINVAL;
        ctrlParms.result = 0;
        printk("board_ioctl: invalid command %x, cmd %d .\n",command,_IOC_NR(command));
        break;

    } /* switch */

    return (ret);

} /* board_ioctl */

/***************************************************************************
* SES Button ISR/GPIO/LED functions.
***************************************************************************/
static Bool sesBtn_pressed(void)
{
	unsigned int intSts = 0, extIntr, value = 0;
	int actHigh = 0;
	Bool pressed = 1;

	if( sesBtn_polling == 0 )
	{
#if defined(CONFIG_BCM96838)
		if ((sesBtn_irq >= INTERRUPT_ID_EXTERNAL_0) && (sesBtn_irq <= INTERRUPT_ID_EXTERNAL_5)) {
#else
		if ((sesBtn_irq >= INTERRUPT_ID_EXTERNAL_0) && (sesBtn_irq <= INTERRUPT_ID_EXTERNAL_3)) {
#endif
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
			intSts = kerSysGetGpioValue(MAP_EXT_IRQ_TO_GPIO( sesBtn_irq - INTERRUPT_ID_EXTERNAL_0));
#elif defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
			intSts = PERF->ExtIrqSts & (1 << (sesBtn_irq - INTERRUPT_ID_EXTERNAL_0 + EI_STATUS_SHFT));
#else
			intSts = PERF->ExtIrqCfg & (1 << (sesBtn_irq - INTERRUPT_ID_EXTERNAL_0 + EI_STATUS_SHFT));
#endif

		}
		else
			return 0;

		extIntr = extIntrInfo[sesBtn_irq-INTERRUPT_ID_EXTERNAL_0];
		actHigh = IsExtIntrTypeActHigh(extIntr);

		if( ( actHigh && intSts ) || (!actHigh && !intSts ) )
		{
			//check the gpio status here too if shared.
			if( IsExtIntrShared(extIntr) )
			{
				 value = kerSysGetGpioValue(sesBtn_gpio);
				 if( (value&&!actHigh) || (!value&&actHigh) )
					 pressed = 0;
			}
		}
		else
			pressed = 0;
	}
	else
	{
		pressed = 0;
		if( sesBtn_gpio != BP_NOT_DEFINED )
		{
			actHigh = sesBtn_gpio&BP_ACTIVE_LOW ? 0 : 1;
			value = kerSysGetGpioValue(sesBtn_gpio);
		    if( (value&&actHigh) || (!value&&!actHigh) )
			    pressed = 1;
		}
	}

    return pressed;
}

static void sesBtn_timer_handler(unsigned long arg)
{
    unsigned long currentJiffies = jiffies;
    if ( sesBtn_pressed() ) {
        doPushButtonHold(PB_BUTTON_1, currentJiffies);
        mod_timer(&sesBtn_timer, currentJiffies + msecs_to_jiffies(100)); 
    }
    else {
        atomic_set(&sesBtn_active, 0);
        doPushButtonRelease(PB_BUTTON_1, currentJiffies);
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        BcmHalExternalIrqUnmask(sesBtn_irq);
#else
        BcmHalInterruptEnable(sesBtn_irq);
#endif
    }
}


void sesBtn_defaultAction(unsigned long time, unsigned long param) {
    wake_up_interruptible(&g_board_wait_queue);
}

static irqreturn_t sesBtn_isr(int irq, void *dev_id)
{
    int ext_irq_idx = 0, value=0;
    irqreturn_t ret = IRQ_NONE;
    unsigned long currentJiffies = jiffies;

    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
    {
        value = kerSysGetGpioValue(*(int *)dev_id);
        if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
        {
            ret = IRQ_HANDLED;
        }
    }
    else
    {
        ret = IRQ_HANDLED;
    }

    if (IRQ_HANDLED == ret) {   
        int timerSet = mod_timer(&sesBtn_timer, (currentJiffies + msecs_to_jiffies(100)));	/* 100 msec */
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        BcmHalExternalIrqMask(irq);
#endif
        if ( 0 == timerSet ) { 
            atomic_set(&sesBtn_active, SES_BTN_LEGACY);
            doPushButtonPress(PB_BUTTON_1, currentJiffies);
        }
    }

#if !defined(CONFIG_BCM_6802_MoCA) && !defined(CONFIG_ARM)
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx])) {
        BcmHalInterruptEnable(sesBtn_irq);
    }
#endif

    return ret;
}

// return 1 if interrupt was mapped.  Return 0 otherwise
static int __init sesBtn_mapIntr(int context)
{
    int ret = 0;
    int ext_irq_idx;

    if( BpGetWirelessSesExtIntr(&sesBtn_irq) == BP_SUCCESS )
    {
        BpGetWirelessSesExtIntrGpio(&sesBtn_gpio);
        if( sesBtn_irq != BP_EXT_INTR_NONE )
        {
#if defined(CONFIG_BCM960333)
            if( sesBtn_gpio != BP_NOT_DEFINED && sesBtn_irq != BP_EXT_INTR_NONE) 
                mapBcm960333GpioToIntr(sesBtn_gpio & BP_GPIO_NUM_MASK, sesBtn_irq);
#endif      
            printk("SES: Button Interrupt 0x%x is enabled\n", sesBtn_irq);
        }
        else
        {
            if( sesBtn_gpio != BP_NOT_DEFINED )
            {
                printk("SES: Button Polling is enabled on gpio %x\n", sesBtn_gpio);
                kerSysSetGpioDirInput(sesBtn_gpio);
                sesBtn_polling = 1;
            }
        }
    }
    else
        return 0;

    if( sesBtn_irq != BP_EXT_INTR_NONE )
    {
        ext_irq_idx = (sesBtn_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
#if defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
        kerSysInitPinmuxInterface(BP_PINMUX_FNTYPE_IRQ | ext_irq_idx);
#endif
        if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
        {
            static int dev = -1;
            int hookisr = 1;

            if (IsExtIntrShared(sesBtn_irq))
            {
                /* get the gpio and make it input dir */
                if( sesBtn_gpio != BP_NOT_DEFINED )
                {
                    sesBtn_gpio &= BP_GPIO_NUM_MASK;;
                    printk("SES: Button Interrupt gpio is %d\n", sesBtn_gpio);
                    kerSysSetGpioDirInput(sesBtn_gpio);
                    dev = sesBtn_gpio;
                }
                else
                {
                    printk("SES: Button Interrupt gpio definition not found \n");
                    hookisr = 0;
                }
            }

            if(hookisr)
            {
                init_timer(&sesBtn_timer);
                sesBtn_timer.function = sesBtn_timer_handler;
                sesBtn_timer.expires  = jiffies + msecs_to_jiffies(100);	/* 100 msec */
                sesBtn_timer.data     = 0;
                atomic_set(&sesBtn_active, 0);
                atomic_set(&sesBtn_forced, 0);
                sesBtn_irq = map_external_irq (sesBtn_irq);
                ret = 1;
                BcmHalMapInterrupt((FN_HANDLER)sesBtn_isr, (unsigned int)&dev, sesBtn_irq);
#if !defined(CONFIG_ARM)
                BcmHalInterruptEnable(sesBtn_irq);
#endif
            }
        }
    }

    return ret;
}

#if defined(WIRELESS)
unsigned long gSesBtnEvOutstanding = 0;
unsigned long gLastSesBtnEvTime;

static unsigned int sesBtn_poll(struct file *file, struct poll_table_struct *wait)
{
    // this is called by the wireless driver to determine if the button is down.  If
    // we are using the new button method, we simply check if the button trigger 
    // occured within the last second.   Otherwise, we fall through to check the 
    // original checks.
    if (gSesBtnEvOutstanding) {
        if (time_after(gLastSesBtnEvTime+HZ, jiffies)) {            
            return POLLIN;
        } else {
            atomic_set(&sesBtn_active, 0);
            gSesBtnEvOutstanding = 0;
        }
         
    }
    
    if ( sesBtn_polling ) {
        if ( sesBtn_pressed() ) {
            return POLLIN;
        }
    }
    else if (atomic_read(&sesBtn_active)) {
        return POLLIN;
    }
    else if (atomic_read(&sesBtn_forced)) {
        atomic_set(&sesBtn_forced,0);
        return POLLIN;
    }
    return 0;
}

static ssize_t sesBtn_read(struct file *file,  char __user *buffer, size_t count, loff_t *ppos)
{
    volatile unsigned int event=0;
    ssize_t ret=0;
    unsigned long flags;
    int exit = 0;

    /* Synchronization note: This code does a non-atomic test and set of
     * sesBtn_active that could cause a race-condition with btnHook_Ses,
     * so this must be protected with sesBtn_newapi_spinlock.
     */

    /* New button API: Return the type of SES button press (Short/Long) */
    spin_lock_irqsave(&sesBtn_newapi_spinlock, flags);
    if (atomic_read(&sesBtn_active) == SES_BTN_AP) {
        event = SES_EVENTS | SES_EVENT_BTN_AP;
        atomic_set(&sesBtn_active, 0);
    }
    else if (atomic_read(&sesBtn_active) == SES_BTN_STA) {
        event = SES_EVENTS | SES_EVENT_BTN_STA;
        atomic_set(&sesBtn_active, 0);
    }
    /* Legacy button API: Return a simple flag (SES_EVENTS) and let the
     * userspace code call read repeatedly to calculate the press time
     */
    else {
        if (sesBtn_polling) {
            if (0 == sesBtn_pressed()) {
                exit = 1;
            }
        }
        else if (0 == atomic_read(&sesBtn_active)) {
            exit = 1;
        }
        event = SES_EVENTS;
    }
    spin_unlock_irqrestore(&sesBtn_newapi_spinlock, flags);

    if (exit)
        return ret;

    gSesBtnEvOutstanding = 0;

    __copy_to_user((char*)buffer, (char*)&event, sizeof(event));
    count -= sizeof(event);
    buffer += sizeof(event);
    ret += sizeof(event);
    return ret;
}
#endif /* WIRELESS */


void kerSysMocaHostIntrReset(int dev)
{
    PMOCA_INTR_ARG pMocaInt;
    unsigned long flags;

    if( dev >=  mocaChipNum )
    {
        printk("kerSysMocaHostIntrReset: Error, invalid dev %d\n", dev);
        return;
    }

    spin_lock_irqsave(&mocaint_spinlock, flags);
    pMocaInt = &mocaIntrArg[dev];
    atomic_set(&pMocaInt->disableCount, 0);
    spin_unlock_irqrestore(&mocaint_spinlock, flags);
}

void kerSysRegisterMocaHostIntrCallback(MocaHostIntrCallback callback, void * userArg, int dev)
{
    int ext_irq_idx;
    unsigned short  mocaHost_irq;
    PBP_MOCA_INFO  pMocaInfo;
    PMOCA_INTR_ARG pMocaInt;
    unsigned long flags;

    if( dev >=  mocaChipNum )
    {
        printk("kerSysRegisterMocaHostIntrCallback: Error, invalid dev %d\n", dev);
        return;
    }

    pMocaInfo = &mocaInfo[dev];
    mocaHost_irq = pMocaInfo->intr[BP_MOCA_HOST_INTR_IDX];
    if( mocaHost_irq == BP_NOT_DEFINED )
    {
        printk("kerSysRegisterMocaHostIntrCallback: Error, no mocaHost_irq defined in boardparms\n");    
        return;
    }

    printk("kerSysRegisterMocaHostIntrCallback: mocaHost_irq = 0x%x, is_mocaHostIntr_shared=%d\n", mocaHost_irq, IsExtIntrShared(mocaHost_irq));

    ext_irq_idx = (mocaHost_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
    if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
    {
        int hookisr = 1;

        pMocaInt = &mocaIntrArg[dev];
        pMocaInt->dev = dev;
        pMocaInt->intrGpio = -1;
        pMocaInt->userArg = userArg;
        pMocaInt->mocaCallback = callback;
        if (IsExtIntrShared(mocaHost_irq))
        {
            /* get the gpio and make it input dir */
            unsigned short gpio;
            if( (gpio = pMocaInfo->intrGpio[BP_MOCA_HOST_INTR_IDX]) != BP_NOT_DEFINED )
            {
                gpio &= BP_GPIO_NUM_MASK;
                printk("MoCA Interrupt gpio is %d\n", gpio);
                kerSysSetGpioDirInput(gpio);
                pMocaInt->intrGpio = gpio;
            }
            else
            {
                  printk("MoCA interrupt gpio definition not found \n");
                  /* still need to hook it because the early bhr board does not have
                   * gpio pin for the shared LAN moca interrupt
                   */
                  hookisr = 1;
            }
        }

        spin_lock_irqsave(&mocaint_spinlock, flags);
        atomic_set(&pMocaInt->disableCount, 0);
        if(hookisr)
        {
            pMocaInt->irq = map_external_irq(mocaHost_irq);
            BcmHalMapInterrupt((FN_HANDLER)mocaHost_isr, (unsigned int)pMocaInt, pMocaInt->irq);
#if !defined(CONFIG_ARM)
            BcmHalInterruptDisable(pMocaInt->irq);
#else
            BcmHalExternalIrqMask(pMocaInt->irq);
#endif
        }
        spin_unlock_irqrestore(&mocaint_spinlock, flags);
    }
}

void kerSysMocaHostIntrEnable(int dev)
{
    PMOCA_INTR_ARG  pMocaInt;
    unsigned long flags;

    spin_lock_irqsave(&mocaint_spinlock, flags);
    if( dev <  mocaChipNum )
    {
        pMocaInt = &mocaIntrArg[dev];

        if (atomic_dec_return(&pMocaInt->disableCount) <= 0)
        {
#if !defined(CONFIG_ARM)
           BcmHalInterruptEnable(pMocaInt->irq);
#else
           BcmHalExternalIrqUnmask(pMocaInt->irq);
#endif
        }
    }
    spin_unlock_irqrestore(&mocaint_spinlock, flags);
}

void kerSysMocaHostIntrDisable(int dev)
{
    PMOCA_INTR_ARG  pMocaInt;
    int i;
    unsigned long flags;

    spin_lock_irqsave(&mocaint_spinlock, flags);
    if( dev <  mocaChipNum )
    {
        pMocaInt = &mocaIntrArg[dev];

        atomic_inc(&pMocaInt->disableCount);

        for (i=0; i<BP_MOCA_MAX_NUM; i++)
        {
            if ((i != dev) &&
                (mocaIntrArg[i].irq == pMocaInt->irq) &&
                (atomic_read(&mocaIntrArg[i].disableCount) <= 0))
            {
                // Don't disable this interrupt. It's shared and
                // the other MoCA interface still needs it. 
                spin_unlock_irqrestore(&mocaint_spinlock, flags);
                return;
            }
        }

#if !defined(CONFIG_ARM)
        BcmHalInterruptDisable(pMocaInt->irq);
#else
        BcmHalExternalIrqMask(pMocaInt->irq);
#endif
    }
    spin_unlock_irqrestore(&mocaint_spinlock, flags);
}

static irqreturn_t mocaHost_isr(int irq, void *dev_id)
{
    PMOCA_INTR_ARG pMocaIntrArg = (PMOCA_INTR_ARG)dev_id;
    int isOurs = 1;
    PBP_MOCA_INFO pMocaInfo;
    int ext_irq_idx = 0, value=0, valueReset = 0, valueMocaW = 0;
    unsigned short gpio;

    //printk("mocaHost_isr called for chip %d, irq %d, gpio %d\n", pMocaIntrArg->dev, irq, pMocaIntrArg->intrGpio);
    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;

    /* When MoCA and SES button share the interrupt, the MoCA handler must be called
       so that the interrupt is re-enabled */
#if defined (WIRELESS)
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]) && (irq != sesBtn_irq))
#else
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
#endif
    {
        if( pMocaIntrArg->intrGpio != -1 )
        {
            value = kerSysGetGpioValue(pMocaIntrArg->intrGpio);
            if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
                isOurs = 1;
            else
                isOurs = 0;
        }
        else
        {
            /* for BHR board, the L_HOST_INTR does not have gpio pin. this really sucks! have to check all other interrupt sharing gpio status,
             * if they are not triggering, then it is L_HOST_INTR.  next rev of the board will add gpio for L_HOST_INTR. in the future, all the
             * shared interrupt will have a dedicated gpio pin.
             */
            if( resetBtn_gpio != BP_NOT_DEFINED )
                valueReset = kerSysGetGpioValue(resetBtn_gpio);

               pMocaInfo = &mocaInfo[BP_MOCA_TYPE_WAN];
            if( (gpio = pMocaInfo->intrGpio[BP_MOCA_HOST_INTR_IDX]) != BP_NOT_DEFINED )
                valueMocaW = kerSysGetGpioValue(gpio);

            if( IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) )
            {
                if( (value = (valueReset|valueMocaW)) )
                    isOurs = 0;
            }
            else
            {
                if( (value = (valueReset&valueMocaW)) == 0 )
                    isOurs = 0;
            }

            //printk("BHR board moca_l interrupt: reset %d:%d, ses %d:%d, moca_w %d:%d, isours %d\n", resetBtn_gpio, valueReset,
            //    sesBtn_gpio, valueSes, gpio&BP_GPIO_NUM_MASK, valueMocaW, isOurs);
        }
    }

    if (isOurs)
    {
       if (atomic_read(&pMocaIntrArg->disableCount) <= 0)
       {
          pMocaIntrArg->mocaCallback(irq, pMocaIntrArg->userArg);
          return IRQ_HANDLED;
       }
    }

    return IRQ_NONE;
}

#if defined(WIRELESS)
static void __init sesLed_mapGpio()
{
    if( BpGetWirelessSesLedGpio(&sesLed_gpio) == BP_SUCCESS )
    {
        printk("SES: LED GPIO 0x%x is enabled\n", sesLed_gpio);
    }
}

#if defined(GPL_CODE_VOIP_LED)
static void AEI_VoipLed_ctrl(char *action)
{
    char * p = NULL;
    int line = 0;
    int status = 0;

    p = action;

    line = *p - 48 + kLedVoip1;
    p = p+2;
    status = *p - 48;

    if (status == kLedStateAmber){
        kerSysLedCtrl(line, kLedStateOn);
        kerSysLedCtrl(line, kLedStateOff);
    }
    else{
        kerSysLedCtrl(line, status);
    }

    return;
}
#endif
#if defined(GPL_CODE)
static void AEI_wlanLed_ctrl(int action)
{
    if (action == kLedStateAmber){
        kerSysLedCtrl(AEI_kLedWlanGreen, kLedStateOn);
        kerSysLedCtrl(AEI_kLedWlanRed, kLedStateOn);
        kerSysLedCtrl(AEI_kLedWlanAct, kLedStateOn);
    }
    else if (action == kLedStateOff) {
        kerSysLedCtrl(AEI_kLedWlanGreen, kLedStateOff);
        kerSysLedCtrl(AEI_kLedWlanRed, kLedStateOff);
        kerSysLedCtrl(AEI_kLedWlanAct, kLedStateOff);
    }
    else
        kerSysLedCtrl(AEI_kLedWlan, action);

    return;
}
#endif
#if defined(GPL_CODE_CHECK_FLASH_ID)
static int  AEI_get_flash_mafId(void)
{   struct mtd_info *mtd=NULL;
    int  mafId=0;
    mtd=get_mtd_device_nm("tag");
    if(mtd!=NULL)
    {
      mafId=mtd->get_fact_prot_info(mtd,NULL,0);
      put_mtd_device(mtd);
    }
    else
      printk("mtd is NULL\n");
    return mafId;
}
#endif
static void sesLed_ctrl(int action)
{
    char blinktype = ((action >> 24) & 0xff); /* extract blink type for SES_LED_BLINK  */

    BOARD_LED_STATE led;

    if(sesLed_gpio == BP_NOT_DEFINED)
        return;

#if defined(GPL_CODE)
    {
    char status = ((action >> 8) & 0xff); /* extract status */
    char event = ((action >> 16) & 0xff); /* extract event */

    if ((action & 0xff) == SES_LED_OFF) { /* extract led */
       kerSysLedCtrl(kLedSes, kLedStateOff);
       printk("< SES_LED_OFF >\n");
       return;
    }

    switch ((int) event) {
        case   WSC_EVENTS_PROC_IDLE:
            printk("< WSC_EVENTS_PROC_IDLE >\n");
            kerSysLedCtrl(kLedSes, kLedStateOn);
            break;
        case    WSC_EVENTS_PROC_WAITING:
            printk("< WSC_EVENTS_PROC_WAITING >\n");
            kerSysLedCtrl(kLedSes, kLedStateOn);
            kerSysLedCtrl(kLedSes, kLedStateUserWpsInProgress);
            break;
        case    WSC_EVENTS_PROC_SUCC:
            printk("< WSC_EVENTS_PROC_SUCC >\n");
            kerSysLedCtrl(kLedSes, kLedStateOn);
            break;
        case    WSC_EVENTS_PROC_FAIL:
            printk("< WSC_EVENTS_PROC_FAIL > status=%d\n", status);
            if ( status == 0 ) {
                kerSysLedCtrl(kLedSes, kLedStateFail);
                kerSysLedCtrl(kLedSes, kLedStateSlowBlinkContinues);
            }
            else if ( status == 1 ) {
                kerSysLedCtrl(kLedSes, kLedStateFail);
            }
            break;
        case    WSC_EVENTS_PROC_PBC_OVERLAP:
            printk("< WSC_EVENTS_PROC_PBC_OVERLAP >\n");
            if ( status == 0 ) {
                kerSysLedCtrl(kLedSes, kLedStateOn);
                kerSysLedCtrl(kLedSes, kLedStateSlowBlinkContinues);
            }
            else if ( status == 1 )
                kerSysLedCtrl(kLedSes, kLedStateOn);
            break;
        default:
            printk("< WSC_EVENTS_UNRECGNIZED >\n");
//            SetGpio(30,0);
//            SetGpio(23,0);
           //kerSysLedCtrl(kLedSes, kLedStateSlowBlinkContinues);
    }//switch
    }//block
#else


    action &= 0xff; /* extract led */

    switch (action) {
    case SES_LED_ON:
        led = kLedStateOn;
        break;
    case SES_LED_BLINK:
        if(blinktype)
            led = blinktype;
        else
            led = kLedStateSlowBlinkContinues;
        break;
    case SES_LED_OFF:
    default:
        led = kLedStateOff;
    }

    kerSysLedCtrl(kLedSes, led);
#endif //(GPL_CODE)
}
#endif

static void __init ses_board_init()
{
    int ret;
    ret = sesBtn_mapIntr(0);
    if (ret) {
        registerPushButtonPressNotifyHook(PB_BUTTON_1, sesBtn_defaultAction, 0);
#if defined(SUPPORT_IEEE1905)
        //1905 is triggered by the plc uke button action.  Attach hook to button 1 if using
        //the old style of board parms
        registerPushButtonPressNotifyHook(PB_BUTTON_1, btnHook_PlcUke, 0);
#endif
    }
#if defined(WIRELESS)
    sesLed_mapGpio();
#endif
}

static void __exit ses_board_deinit()
{
    if( sesBtn_polling == 0 && sesBtn_irq != BP_NOT_DEFINED )
    {
        int ext_irq_idx = sesBtn_irq - INTERRUPT_ID_EXTERNAL_0;
        if(sesBtn_irq) {
            del_timer(&sesBtn_timer);
            atomic_set(&sesBtn_active, 0);
            atomic_set(&sesBtn_forced, 0);
            if (!IsExtIntrShared(extIntrInfo[ext_irq_idx])) {
                BcmHalInterruptDisable(sesBtn_irq);
            }
        }
    }
}

#if defined CONFIG_BCM963138 || defined CONFIG_BCM963148
static void __init nfc_board_init(void)
{
    unsigned short nfc_irq;
    int ext_irq_idx;

    if (BpGetNfcExtIntr(&nfc_irq) != BP_SUCCESS ||
	nfc_irq == BP_EXT_INTR_NONE)
	return;

    ext_irq_idx = (nfc_irq & ~BP_EXT_INTR_FLAGS_MASK) - BP_EXT_INTR_0;
    printk("NFC: Interrupt %d enabled\n", ext_irq_idx);
    kerSysInitPinmuxInterface(BP_PINMUX_FNTYPE_IRQ | ext_irq_idx);
}
#endif

/***************************************************************************
* Dying gasp ISR and functions.
***************************************************************************/

/* For any driver running on another cpu that needs to know if system is losing
   power */
int kerSysIsDyingGaspTriggered(void)
{
    return isDyingGaspTriggered;
}


#if !defined(CONFIG_BCM960333)
static irqreturn_t kerSysDyingGaspIsr(int irq, void * dev_id)
{
#if defined(CONFIG_BCM96318)
    unsigned short plcGpio;
#endif
    struct list_head *pos;
    CB_DGASP_LIST *tmp = NULL, *dslOrGpon = NULL;
    unsigned short usPassDyingGaspGpio;		// The GPIO pin to propogate a dying gasp signal

    isDyingGaspTriggered = 1;
    UART->Data = 'D';
    UART->Data = '%';
    UART->Data = 'G';

#if defined (WIRELESS)
    kerSetWirelessPD(WLAN_OFF);
#endif
    /* first to turn off everything other than dsl or gpon */
    list_for_each(pos, &g_cb_dgasp_list_head->list) {
        tmp = list_entry(pos, CB_DGASP_LIST, list);
        if(strncmp(tmp->name, "dsl", 3) && strncmp(tmp->name, "gpon", 4)) {
            (tmp->cb_dgasp_fn)(tmp->context);
        }else {
            dslOrGpon = tmp;
        }
    }
	
    // Invoke dying gasp handlers
    if(dslOrGpon)
        (dslOrGpon->cb_dgasp_fn)(dslOrGpon->context);

    /* reset and shutdown system */


#if defined (CONFIG_BCM96318)
    /* Use GPIO to control the PLC and wifi chip reset on 6319 PLC board*/
    if( BpGetPLCPwrEnGpio(&plcGpio) == BP_SUCCESS )
    {
    	kerSysSetGpioState(plcGpio, kGpioInactive);
    }
#endif

    /* Set WD to fire in 1 sec in case power is restored before reset occurs */
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
    bcm_suspend_watchdog();
#endif
    start_watchdog(1000000, 1);

	// If configured, propogate dying gasp to other processors on the board
	if(BpGetPassDyingGaspGpio(&usPassDyingGaspGpio) == BP_SUCCESS)
	    {
	    // Dying gasp configured - set GPIO
	    kerSysSetGpioState(usPassDyingGaspGpio, kGpioInactive);
	    }

    // If power is going down, nothing should continue!
    while (1);
	return( IRQ_HANDLED );
}
#endif /* !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)*/

#if !defined(CONFIG_BCM960333)
static int dg_enabled = 0;
void kerSysDisableDyingGaspInterrupt( void )
{
    mutex_lock(&dgaspMutex);

    if (!dg_enabled) {
        mutex_unlock(&dgaspMutex);
        return;
    }

    BcmHalInterruptDisable(INTERRUPT_ID_DG);
    printk("DYING GASP IRQ Disabled\n");
    dg_enabled = 0;
    mutex_unlock(&dgaspMutex);
}
EXPORT_SYMBOL(kerSysDisableDyingGaspInterrupt);

void kerSysEnableDyingGaspInterrupt( void )
{
    static int dg_mapped = 0;
    int dg_active;    

    mutex_lock(&dgaspMutex);
    
    /* Ignore requests to enable DG if it is already enabled */
    if (dg_enabled) {
        mutex_unlock(&dgaspMutex);
        return;
    }

    /* Set DG Parameters */
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381)
    msleep(5);
    /* Setup dying gasp threshold @ 1.25V with 0mV Heysteresis */
    DSLPHY_AFE->BgBiasReg[0] = (DSLPHY_AFE->BgBiasReg[0] & ~0xffff) | 0x04cd;
    DSLPHY_AFE->AfeReg[0] = (DSLPHY_AFE->AfeReg[0] & ~0xffff) | 0x002F;
    msleep(5);
#endif /* defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) */

    /* Check if DG is already active */
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    dg_active = 0;
#elif defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    dg_active =  PERF->IrqStatus[0] & (1 << (INTERRUPT_ID_DG - ISR_TABLE_OFFSET));
#elif defined(CONFIG_BCM963381)
    dg_active =  (PERF->IrqStatus & (((uint64)1) << (INTERRUPT_ID_DG - INTERNAL_ISR_TABLE_OFFSET)))? 1 : 0;
#else
    dg_active =  PERF->IrqControl[0].IrqStatus & (1 << (INTERRUPT_ID_DG - INTERNAL_ISR_TABLE_OFFSET));
#endif

    /* Enable/Install DG Interrupt */
    if (dg_active) 
    {
        printk("DYING GASP active already -- disabled\n");
    } 
    else if (kerSysIsBatteryEnabled()) 
    {
        printk("DYING GASP enabling postponed until battery is initialised\n");
    } 
    else 
    {        
        if (dg_mapped) 
        { 
            BcmHalInterruptEnable(INTERRUPT_ID_DG);
            printk("DYING GASP IRQ Enabled\n");
        }
        else 
        {
            BcmHalMapInterrupt((FN_HANDLER)kerSysDyingGaspIsr, 0, INTERRUPT_ID_DG);	    
#if !defined(CONFIG_ARM)
            BcmHalInterruptEnable( INTERRUPT_ID_DG );
#endif
            dg_mapped = 1;
            printk("DYING GASP IRQ Initialized and Enabled\n");
        }
        dg_enabled = 1;
    }
    mutex_unlock(&dgaspMutex);
}
EXPORT_SYMBOL(kerSysEnableDyingGaspInterrupt);
#endif /* !defined(CONFIG_BCM960333) */

static void __init kerSysInitDyingGaspHandler( void )
{
    CB_DGASP_LIST *new_node;

    if( g_cb_dgasp_list_head != NULL) {
        printk("Error: kerSysInitDyingGaspHandler: list head is not null\n");
        return;
    }
    new_node= (CB_DGASP_LIST *)kmalloc(sizeof(CB_DGASP_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_DGASP_LIST));
    INIT_LIST_HEAD(&new_node->list);
    g_cb_dgasp_list_head = new_node;

#if !defined(CONFIG_BCM960333)    
    /* Disable DG Interrupt */
    kerSysDisableDyingGaspInterrupt();

#if defined(CONFIG_BCM96838)
    GPIO->dg_control |= (1 << DG_EN_SHIFT);    
#elif defined(CONFIG_BCM96848)
    MISC_REG->DgSensePadCtrl |= (1 << DG_EN_SHIFT); 
#else
    {
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381)
        pmc_dsl_power_up();
        pmc_dsl_core_reset();
#endif
    }
#endif /* defined(CONFIG_BCM96838) */

    /* Enable DG Interrupt */
    kerSysEnableDyingGaspInterrupt();
#endif /* !defined(CONFIG_BCM960333) */   
} 

static void __exit kerSysDeinitDyingGaspHandler( void )
{
    struct list_head *pos;
    CB_DGASP_LIST *tmp;

    if(g_cb_dgasp_list_head == NULL)
        return;

    list_for_each(pos, &g_cb_dgasp_list_head->list) {
        tmp = list_entry(pos, CB_DGASP_LIST, list);
        list_del(pos);
        kfree(tmp);
    }

    kfree(g_cb_dgasp_list_head);
    g_cb_dgasp_list_head = NULL;

} /* kerSysDeinitDyingGaspHandler */

void kerSysRegisterDyingGaspHandler(char *devname, void *cbfn, void *context)
{
    CB_DGASP_LIST *new_node;

    // do all the stuff that can be done without the lock first
    if( devname == NULL || cbfn == NULL ) {
        printk("Error: kerSysRegisterDyingGaspHandler: register info not enough (%s,%x,%x)\n", devname, (unsigned int)cbfn, (unsigned int)context);
        return;
    }

    if (strlen(devname) > (IFNAMSIZ - 1)) {
        printk("Warning: kerSysRegisterDyingGaspHandler: devname too long, will be truncated\n");
    }

    new_node= (CB_DGASP_LIST *)kmalloc(sizeof(CB_DGASP_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_DGASP_LIST));
    INIT_LIST_HEAD(&new_node->list);
    strncpy(new_node->name, devname, IFNAMSIZ-1);
    new_node->cb_dgasp_fn = (cb_dgasp_t)cbfn;
    new_node->context = context;

    // OK, now acquire the lock and insert into list
    mutex_lock(&dgaspMutex);
    if( g_cb_dgasp_list_head == NULL) {
        printk("Error: kerSysRegisterDyingGaspHandler: list head is null\n");
        kfree(new_node);
    } else {
        list_add(&new_node->list, &g_cb_dgasp_list_head->list);
        printk("dgasp: kerSysRegisterDyingGaspHandler: %s registered \n", devname);
    }
    mutex_unlock(&dgaspMutex);

    return;
} /* kerSysRegisterDyingGaspHandler */

void kerSysDeregisterDyingGaspHandler(char *devname)
{
    struct list_head *pos;
    CB_DGASP_LIST *tmp;
    int found=0;

    if(devname == NULL) {
        printk("Error: kerSysDeregisterDyingGaspHandler: devname is null\n");
        return;
    }

    printk("kerSysDeregisterDyingGaspHandler: %s is deregistering\n", devname);

    mutex_lock(&dgaspMutex);
    if(g_cb_dgasp_list_head == NULL) {
        printk("Error: kerSysDeregisterDyingGaspHandler: list head is null\n");
    } else {
        list_for_each(pos, &g_cb_dgasp_list_head->list) {
            tmp = list_entry(pos, CB_DGASP_LIST, list);
            if(!strcmp(tmp->name, devname)) {
                list_del(pos);
                kfree(tmp);
                found = 1;
                printk("kerSysDeregisterDyingGaspHandler: %s is deregistered\n", devname);
                break;
            }
        }
        if (!found)
            printk("kerSysDeregisterDyingGaspHandler: %s not (de)registered\n", devname);
    }
    mutex_unlock(&dgaspMutex);

    return;
} /* kerSysDeregisterDyingGaspHandler */


/***************************************************************************
 *
 *
 ***************************************************************************/
static int ConfigCs (BOARD_IOCTL_PARMS *parms)
{
    int                     retv = 0;
    return( retv );
}

#if defined(GPL_CODE)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
static void reboot_thread(struct work_struct *work)
#else
static void reboot_thread(void *arg)
#endif
{
    AEI_SaveSyslogOnReboot();
    kerSysMipsSoftReset();
    return;
}
#endif /* GPL_CODE */

/***************************************************************************
* Handle push of restore to default button
***************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
static void restore_to_default_thread(struct work_struct *work)
#else
static void restore_to_default_thread(void *arg)
#endif
{
#if defined(GPL_CODE_FACTORY_TEST)
    NVRAM_DATA * pNvramData;
    int needUpdate = 0;
#endif
    char buf[256];

    memset(buf, 0, sizeof(buf));

    // Do this in a kernel thread so we don't have any restriction
    printk("Restore to Factory Default Setting ***\n\n");
#ifdef GPL_CODE_DUAL_IMAGE_CONFIG
    if(getBootedValue(1) == BOOTED_PART1_IMAGE)
       AEI_kerSysImagePsiSet(AEI_IMAGE1_PSI,buf,sizeof(buf),0); 
    else
       AEI_kerSysImagePsiSet(AEI_IMAGE2_PSI,buf,sizeof(buf),0);
#endif
    kerSysPersistentSet( buf, sizeof(buf), 0 );

#if defined(GPL_CODE) || defined(GPL_CODE) || defined(SUPPORT_BACKUP_PSI)
    kerSysBackupPsiSet( buf, sizeof(buf), 0 );
#endif

#if defined(GPL_CODE)
#if defined(GPL_CODE_CONFIG_JFFS)
    /*Now in 63268 Chip, we use jffs2 fs. So we need to sync data to flash from buffer before reboot system.*/
    sys_sync();
#endif
#endif /* GPL_CODE */

#if defined(CONFIG_BCM_PLC_BOOT)
    kerSysFsFileSet("/data/plc/plc_pconfig_state", buf, 1);
#endif

    /* empty log file under data */
#if defined(GPL_CODE_BAND_STEERING)
    sys_unlink("/data/absmetric");
    sys_unlink("/data/abslog");
    sys_unlink("/data/abslog_old");
#endif
#if defined(GPL_CODE)
    sys_unlink("/data/brcm_channel_switch_log");
#endif
#if defined(GPL_CODE_CONSOLE)
    sys_unlink("/data/aei_consolg_enable");
#endif
#if defined(GPL_CODE_WPS_LOCKDOWN_LOG)
    sys_unlink("/data/wps_2gAplockDown_log");
    sys_unlink("/data/wps_5gAplockDown_log");
#endif

#if defined(GPL_CODE_FACTORY_TEST)
    /* Clean up Manu Mode and WLAN feature in CFE nvram */
    mutex_lock(&flashImageMutex);
    if (NULL != (pNvramData = readNvramData()))
    {
	if (pNvramData->ManuMode[0] != '0')
        {
            pNvramData->ManuMode[0] = '0';
	    needUpdate = 1;
        }
        if ((unsigned char)(pNvramData->wlanParams[NVRAM_WLAN_PARAMS_LEN-1]))
        {
            pNvramData->wlanParams[NVRAM_WLAN_PARAMS_LEN-1] = 0;
	    needUpdate = 1;
        }
	if (needUpdate)
            writeNvramDataCrcLocked(pNvramData);
        kfree(pNvramData);
    }
    mutex_unlock(&flashImageMutex);
#endif

#if defined(GPL_CODE)
#if defined(GPL_CODE_INTERNET_LED_BEHAVIOR)
    boardLedCtrl(kLedInternet, kLedStateAmberFastBlinkContinues);
    /* sleep Xs for blink YELLOW LED */
    mdelay(FACTORY_BLINK_TIME);
#endif
#endif

    // kernel_restart is a high level, generic linux way of rebooting.
    // It calls a notifier list and lets sub-systems know that system is
    // rebooting, and then calls machine_restart, which eventually
    // calls kerSysMipsSoftReset.
#if defined(GPL_CODE)
    kerSysScratchPadClearAll();
#endif
#if defined(GPL_CODE)
    char restore_info[2]="1";
    kerSysScratchPadSet("Reset_Info",restore_info,sizeof(restore_info));
#endif

    kernel_restart(NULL);

    return;
}


#if defined(GPL_CODE)
#if defined(GPL_CODE_63168_CHIP)
#define RESET_FLAG_LEFT_SHIFT 4
#else
#define RESET_FLAG_LEFT_SHIFT 6
#endif
#endif

#if defined(GPL_CODE)
static void AEI_resetBtnTimerFunc(unsigned long data)
{
#if defined(GPL_CODE_63168_CHIP)
    uint64 flag = ((uint64)0x10000000 << RESET_FLAG_LEFT_SHIFT);
    if (!(GPIO->GPIOio & flag))
#else
    unsigned int flag = 0x00000001;
    if(!(GPIO->GPIOio[1] & flag))
#endif
    {
        holdTime++;

        if (holdTime >= RESET_HOLD_TIME && holdTime <= FACTORY_HOLD_TIME)
        {
            printk("Set power LED color\n");
            kerSysLedCtrl(kLedPower, kLedStateAmber);
        }
        else if (holdTime > FACTORY_HOLD_TIME)
        {
            printk("\r\nThe system is being reset. Hold it longer to get into bootloader...\r\n");
            INIT_WORK(&rebootWork, reboot_thread);
            schedule_work(&rebootWork);
            return;
        }
    }
    else
    {
        if (holdTime < RESET_HOLD_TIME)
        {
            printk("\r\nThe system is being reset. Please wait...\r\n");
            INIT_WORK(&rebootWork, reboot_thread);
            schedule_work(&rebootWork);
            return;
        }
        else if (holdTime >= RESET_HOLD_TIME && holdTime <= FACTORY_HOLD_TIME)
        {
            INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
            schedule_work(&restoreDefaultWork);
            return;
        }
    }

    pTimer->expires = jiffies + HZ * RESET_POLL_TIME;
    add_timer(pTimer);

    return;
}
#elif defined(GPL_CODE)
/* BA wants reset button to only restore default and not pause at cfe so just keep waiting until button released unless we change cfe also */
static void AEI_resetBtnTimerFunc(unsigned long data)
{
#if defined(GPL_CODE_63168_CHIP)
    uint64 flag = ((uint64)0x10000000 << RESET_FLAG_LEFT_SHIFT);

    if ((GPIO->GPIOio & flag))
    {
         printk("\n*** reset button released***\n");
         INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
         schedule_work(&restoreDefaultWork);
    }
    else
    {
         printk("\n*** Still holding reset button ***\n");
         pTimer->expires = jiffies + HZ*RESET_POLL_TIME;
         add_timer(pTimer);
    }
#elif defined(GPL_CODE_63138_CHIP)
    unsigned int flag = 0x00000001;

    if((GPIO->GPIOio[1] & flag)) //release
    {
         printk("\r\nThe system is being reset. Please wait...\r\n");

         INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
         schedule_work(&restoreDefaultWork);
         return;
    }
    else
    {
         printk("\n*** Still holding reset button ***\n");
         pTimer->expires = jiffies + HZ*RESET_POLL_TIME;
         add_timer(pTimer);
    }
#endif

    return;
}
#elif defined(GPL_CODE)
/* Telus requirement:  If reset button is held up to 10 seconds, just reboot.  If held more than 10 seconds restore default
*/
static void AEI_resetBtnTimerFunc(unsigned long data)
{
#if defined(GPL_CODE_63168_CHIP)
    uint64 flag = ((uint64)0x10000000 << RESET_FLAG_LEFT_SHIFT);

    if (holdTime < FACTORY_HOLD_TIME)
    {
        if(!(GPIO->GPIOio & flag))
        {
            holdTime++;
        }
        else
        {
            printk("\n*** Reset held for %d seconds so not restoring factory default ***\n\n", holdTime);
            holdTime = 0;
#if defined(GPL_CODE_T2200H)
            /*
             * Bill said on Agile ID 117672:
             * If reset button is held less than 4 seconds, do nothing.
             * If held more than 4 seconds turn orange power led, start factory reset after release reset button.
             */
            if (rirq != BP_NOT_DEFINED)
               BcmHalInterruptEnable(rirq);
            return;
#else
            kerSysMipsSoftReset();
#endif
        }
    }
    else
    {
#if defined(GPL_CODE_T2200H)
        if (!(GPIO->GPIOio & flag))
        {
            holdTime++;
        }
        if (holdTime == FACTORY_HOLD_TIME)
        {
            printk("Set power LED color\n");
            kerSysLedCtrl(kLedPower, kLedStateAmber);
            INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
            schedule_work(&restoreDefaultWork);
            return;
        }
        else
        {
            if (!(GPIO->GPIOio & flag))
            {
                printk("Set power LED color\n");
                kerSysLedCtrl(kLedPower, kLedStateAmber);
            }
            else
            {
                INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
                schedule_work(&restoreDefaultWork);
                return;
            }
        }
#else
        INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
        schedule_work(&restoreDefaultWork);
        return;
#endif
    }
#elif defined(GPL_CODE_63138_CHIP)
    unsigned int flag = 0x00000001;

    if (!(GPIO->GPIOio[1] & flag)) {
        holdTime++;
    }

    if (holdTime < FACTORY_HOLD_TIME) {
        if((GPIO->GPIOio[1] & flag)) { //release

            printk("\n*** Reset held for %d seconds so not restoring factory default ***\n\n", holdTime);
            holdTime = 0;
#if defined(GPL_CODE)
            /*
             * Bill said on Agile ID 117672:
             * If reset button is held less than 4 seconds, do nothing.
             * If held more than 4 seconds turn orange power led, start factory reset after release reset button.
             */
            if (rirq != BP_NOT_DEFINED) {
                BcmHalExternalIrqUnmask(rirq);
            }

            restore_in_progress = 0;
            return;
#else
            kernel_restart(NULL);
#endif
        }
    } else { //holdTime >= FACTORY_HOLD_TIME
        if (holdTime == FACTORY_HOLD_TIME)
            boardLedCtrl(kLedInternet, kLedStateAmberSlowBlinkContinues);

        if ((GPIO->GPIOio[1] & flag)) { //release
            printk("\r\nThe system is being reset. Please wait...\r\n");
            INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
            schedule_work(&restoreDefaultWork);
            return;
        }
    }

    printk(KERN_DEBUG "\n*** Still holding reset button ***\n");
    pTimer->expires = jiffies + HZ * RESET_POLL_TIME;
    add_timer(pTimer);
#endif

    return;
}
#else
static void AEI_resetBtnTimerFunc(unsigned long data)
{
    INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
    schedule_work(&restoreDefaultWork);
    return;
}
#endif

#if defined(GPL_CODE_FACTORY_TEST)
static void AEI_resetBtnFactoryTestTimerFunc(unsigned long data)
{
#if defined(CONFIG_BCM963138)
    unsigned int flag = 0x00000001;
    int status = -1;

    if(GPIO->GPIOio[1] & flag) { /*release*/
	if (rirq != BP_NOT_DEFINED)
	    BcmHalExternalIrqUnmask(rirq);
    restore_in_progress = 0;
    status = AEI_BUTTON_RELEASE;
    kerSysSendtoMonitorTask(AEI_MSG_NETLINK_RESET_BUTTON, (char *)&status, sizeof(status));
    return;
    } else {
        pTimer->expires = jiffies + HZ * RESET_POLL_TIME;
	add_timer(pTimer);
    }
#endif
    return;
}
#endif

static irqreturn_t reset_isr(int irq, void *dev_id)
{
    int isOurs = 1, ext_irq_idx = 0, value=0;
#if defined(GPL_CODE_FACTORY_TEST)
    char manuMode[2] = {0};
    int status = -1;
#endif

    //printk("reset_isr called irq %d, gpio %d 0x%lx\n", irq, *(int *)dev_id, PERF->IrqControl32[0].IrqMaskHi);

    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
    {
       value = kerSysGetGpioValue(*(int *)dev_id);
       if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
    	   isOurs = 1;
       else
    	   isOurs = 0;
    }

    if (isOurs)
    {
    	if( !restore_in_progress )
    	{
            printk("\n***reset button press detected***\n\n");

#if defined(GPL_CODE)
            /* Create a timer which fires every seconds */
            pTimer = &resetBtnTimer;
            init_timer(pTimer);
            pTimer->function = AEI_resetBtnTimerFunc;
            pTimer->data = 0;

#if defined (GPL_CODE) || defined(GPL_CODE)
            rirq = irq;
#endif

#if defined(GPL_CODE_63138_CHIP)
            /* mask resetBtn interrupt to allow timer to work */
            BcmHalExternalIrqMask(irq);
#endif
#if defined(GPL_CODE_FACTORY_TEST)
	    if (g_RMA_status && isInManuMode()) {
		int status = -1;
		pTimer->function = AEI_resetBtnFactoryTestTimerFunc;
		status = AEI_BUTTON_PRESS;
		kerSysSendtoMonitorTask(AEI_MSG_NETLINK_RESET_BUTTON, (char *)&status, sizeof(status));
	    }
	    pTimer->function(0);
#else
            /* Start the timer */
            AEI_resetBtnTimerFunc(0);
#endif
#else
            INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
            schedule_work(&restoreDefaultWork);
#endif
            restore_in_progress  = 1;
     	}
        return IRQ_HANDLED;
     }

     return IRQ_NONE;
}

#if defined(WIRELESS)
/***********************************************************************
* Function Name: kerSysScreenPciDevices
* Description  : Screen Pci Devices before loading modules
***********************************************************************/
static void __init kerSysScreenPciDevices(void)
{
    unsigned short wlFlag;

    if((BpGetWirelessFlags(&wlFlag) == BP_SUCCESS) && (wlFlag & BP_WLAN_EXCLUDE_ONBOARD)) {
        /*
        * scan all available pci devices and delete on board BRCM wireless device
        * if external slot presents a BRCM wireless device
        */
        int foundPciAddOn = 0;
        struct pci_dev *pdevToExclude = NULL;
        struct pci_dev *dev = NULL;

        while((dev=pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev))!=NULL) {
            printk("kerSysScreenPciDevices: 0x%x:0x%x:(slot %d) detected\n", dev->vendor, dev->device, PCI_SLOT(dev->devfn));
            if((dev->vendor == BRCM_VENDOR_ID) &&
                (((dev->device & 0xff00) == BRCM_WLAN_DEVICE_IDS)|| 
                ((dev->device/1000) == BRCM_WLAN_DEVICE_IDS_DEC))) {
                    if(PCI_SLOT(dev->devfn) != WLAN_ONBOARD_SLOT) {
                        foundPciAddOn++;
                    } else {
                        pdevToExclude = dev;
                    }                
            }
        }

#ifdef CONFIG_PCI
        if(((wlFlag & BP_WLAN_EXCLUDE_ONBOARD_FORCE) || foundPciAddOn) && pdevToExclude) {
            printk("kerSysScreenPciDevices: 0x%x:0x%x:(onboard) deleted\n", pdevToExclude->vendor, pdevToExclude->device);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
            pci_remove_bus_device(pdevToExclude);
#else
            __pci_remove_bus_device(pdevToExclude);
#endif
        }
#else
#error ATTEMPT TO COMPILE WIRELESS WITHOUT PCI
#endif
    }
}

#if !defined(CONFIG_BCM968500)
/***********************************************************************
* Function Name: kerSetWirelessPD
* Description  : Control Power Down by Hardware if the board supports
***********************************************************************/
static void kerSetWirelessPD(int state)
{
    unsigned short wlanPDGpio;
    if((BpGetWirelessPowerDownGpio(&wlanPDGpio)) == BP_SUCCESS) {
        if (wlanPDGpio != BP_NOT_DEFINED) {
            if(state == WLAN_OFF)
                kerSysSetGpioState(wlanPDGpio, kGpioActive);
            else
                kerSysSetGpioState(wlanPDGpio, kGpioInactive);
        }
    }
}
#endif
#endif

#if defined(GPL_CODE_DUAL_IMAGE) || defined(GPL_CODE_VER2_DUAL_IMAGE)
static int proc_get_bootimage_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r = 0;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    NVRAM_DATA *pNvramData;
    char bootPartition = BOOT_LATEST_IMAGE;
    char *p;

#if defined(GPL_CODE_VER2_DUAL_IMAGE)
	PFILE_TAG pTag1 = getTagFromPartition(1);
	PFILE_TAG pTag2 = getTagFromPartition(2);
	unsigned long sequence1 = pTag1?simple_strtoul(pTag1->imageSequence, NULL, 10):0;
	unsigned long sequence2 = pTag2?simple_strtoul(pTag2->imageSequence, NULL, 10):0;
#endif

    *eof = 1;

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_KERNEL);
    if (pNvramData == NULL)
    {
       return 0;
    }

    if (AEI_readNvramData(pNvramData) == 0)
    {
        for( p = pNvramData->szBootline; p[2] != '\0'; p++ )
        {
            if( p[0] == 'p' && p[1] == '=' )
            {
                bootPartition = p[2];
                break;
            }
        }

#if defined(GPL_CODE_VER2_DUAL_IMAGE)
//change bootPartition to Parition Order Number
    if( bootPartition == BOOT_LATEST_IMAGE )
        bootPartition = (sequence2 > sequence1) ? BOOT_PREVIOUS_IMAGE : BOOT_LATEST_IMAGE;
    else
        bootPartition = (sequence2 < sequence1) ? BOOT_PREVIOUS_IMAGE : BOOT_LATEST_IMAGE;
    if(pTag1 == NULL && pTag2 != NULL)
        bootPartition = BOOT_PREVIOUS_IMAGE;
    if(pTag1 != NULL && pTag2 == NULL)
        bootPartition = BOOT_LATEST_IMAGE;
#endif

        r += sprintf(page + r, "%c ", bootPartition);
    }

    r += sprintf(page + r, "\n");
    kfree(pNvramData);
    return (r < cnt)? r: 0;
}

static int proc_set_bootimage_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    NVRAM_DATA *pNvramData;
    char input[32];

    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    int bskip = true;
    char *p = NULL;

#if defined(GPL_CODE_VER2_DUAL_IMAGE)
	PFILE_TAG pTag1 = getTagFromPartition(1);
	PFILE_TAG pTag2 = getTagFromPartition(2);
	unsigned long sequence1 = pTag1?simple_strtoul(pTag1->imageSequence, NULL, 10):0;
	unsigned long sequence2 = pTag2?simple_strtoul(pTag2->imageSequence, NULL, 10):0;
	char cBootPartition = BOOT_LATEST_IMAGE;
#endif

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;
    if ((cnt > 2) || (copy_from_user(input, buf, cnt) != 0))
        return -EFAULT;
#ifdef GPL_CODE_NAND_IMG_CHECK
	if(input[0] == '9')
	{
		if(gSetWrongCRC != 1)
		{
			gSetWrongCRC = 1; //1=set wrong crc
			printk("It will set wrong crc when you flash new image before reboot, poweroff, and set 0 or 1 again.\n");
		}
	}
	else
	{
		if(gSetWrongCRC != 0)
		{
			gSetWrongCRC = 0; //0=not set wrong crc
			printk("It will set correct crc when you flash new image.\n");
		}
	}
#endif

    if(input[0]!=BOOT_LATEST_IMAGE && input[0]!=BOOT_PREVIOUS_IMAGE)
        return -EFAULT;

    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_KERNEL);
    if (pNvramData == NULL)
    {
       printk("ERROR - Could not allocate memory for pNvramData (%s)\n", __FUNCTION__);
       return 0;
    }
    memset(pNvramData, 0, sizeof(NVRAM_DATA));

    if (AEI_readNvramData(pNvramData) == 0)
    {
        for( p = pNvramData->szBootline; p[2] != '\0'; p++ )
        {
            if( p[0] == 'p' && p[1] == '=' )
            {
#if defined(GPL_CODE_VER2_DUAL_IMAGE)
//change to really partition that user want to boot.
				if(input[0] == BOOT_LATEST_IMAGE )
					cBootPartition = (sequence2 > sequence1) ? BOOT_PREVIOUS_IMAGE : BOOT_LATEST_IMAGE;
				else
					cBootPartition = (sequence2 < sequence1) ? BOOT_PREVIOUS_IMAGE : BOOT_LATEST_IMAGE;
                if(pTag1 == NULL || pTag2 == NULL)
                    cBootPartition = BOOT_LATEST_IMAGE;
				p[2]=cBootPartition;
#else
                p[2]=input[0];
#endif
                bskip = false;
                break;
            }
        }

        writeNvramDataCrcLocked(pNvramData);
        kfree(pNvramData);
        if (!bskip)
        {
#if defined(GPL_CODE_VER2_DUAL_IMAGE)
            kerSysSetBootImageState(cBootPartition == BOOT_LATEST_IMAGE ? '0' : '1');
#else
            kerSysSetBootImageState(input[0] == BOOT_LATEST_IMAGE ? '0' : '1');
#endif
        }
        return cnt;
    }
    kfree(pNvramData);
    return 0;
}

#endif

#if defined(GPL_CODE)
static int proc_get_other_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r = 0;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    NVRAM_DATA *pNvramData = NULL;

    *eof = 1;

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_KERNEL);
    if (pNvramData == NULL)
    {
       return 0;
    }
    if (AEI_readNvramData(pNvramData) == 0)
    {
	r += snprintf(page + r, length, "%s", &((unsigned char *)pNvramData)[offset]);
    }

    r += sprintf(page + r, "\n");
    kfree(pNvramData);
    return (r < cnt)? r: 0;
}

static int proc_set_other_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    NVRAM_DATA *pNvramData = NULL;
    char input[64] = { 0 };
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    memset(input, 0, 64);
    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;
    if ((cnt  > length ) || (copy_from_user(input, buf, cnt) != 0))
        return -EFAULT;
    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_KERNEL);
    if (pNvramData == NULL)
    {
       printk("ERROR - Could not allocate memory for pNvramData (%s)\n", __FUNCTION__);
       return 0;
    }
    if (AEI_readNvramData(pNvramData) == 0)
    {
        memset(((char *)pNvramData) + offset, 0, length);
	strncpy(((char *)pNvramData) + offset, input, cnt - 1);
        writeNvramDataCrcLocked(pNvramData);
        kfree(pNvramData);
        return cnt;
    }
    kfree(pNvramData);
    return 0;
}
#endif



#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)
static int proc_get_wlan_feature_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r = 0;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    NVRAM_DATA *pNvramData = NULL;
    *eof = 1;

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;
        
    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_ATOMIC);
    if (pNvramData == NULL)
    {
        printk("ERROR - Could not allocate memory for pNvramData (%s)\n", __FUNCTION__);
        return 0;
    }
    memset(((char *)pNvramData),0,sizeof(NVRAM_DATA)); 
    if (AEI_readNvramData(pNvramData) == 0)
    {
        r += snprintf(page + r, 8, "0x%02x\n", ((unsigned char *)pNvramData)[offset]);
    }
    kfree(pNvramData);
    return (r < cnt)? r: 0;
}
static int proc_set_wlan_feature_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    NVRAM_DATA *pNvramData = NULL;
    char input[NVRAM_WLAN_PARAMS_LEN] = { 0 };
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    int wlan_feature=0;

    memset(input, 0, NVRAM_WLAN_PARAMS_LEN);
    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;
    //Only one char is used, so the max of cnt should be <= 5  (0x02)
    if ((copy_from_user(input, buf, cnt) != 0) || cnt > 5)
        return -EFAULT;
    if(!strncmp(input,"0x",2))
    {
        if (kstrtoint(input, 16, &wlan_feature))
        {
            return -EINVAL;
        }
    }else
    {
        if (kstrtoint(input, 10, &wlan_feature))
        {
            return -EINVAL;
        }
    }
    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_ATOMIC);
    if (pNvramData == NULL)
    {
        printk("ERROR - Could not allocate memory for pNvramData (%s)\n", __FUNCTION__);
        return 0;
    }
    memset(((char *)pNvramData),0,sizeof(NVRAM_DATA));
    if (AEI_readNvramData(pNvramData) == 0)
    {
        ((unsigned char *)pNvramData)[offset] = (unsigned char)wlan_feature;
        writeNvramDataCrcLocked(pNvramData);
        kfree(pNvramData);
        return cnt;
    }
    kfree(pNvramData);
    return 0;
}
#endif


extern unsigned char g_blparms_buf[];

/***********************************************************************
 * Function Name: kerSysBlParmsGetInt
 * Description  : Returns the integer value for the requested name from
 *                the boot loader parameter buffer.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysBlParmsGetInt( char *name, int *pvalue )
{
    char *p2, *p1 = g_blparms_buf;
    int ret = -1;

    *pvalue = -1;

    /* The g_blparms_buf buffer contains one or more contiguous NULL termianted
     * strings that ends with an empty string.
     */
    while( *p1 )
    {
        p2 = p1;

        while( *p2 != '=' && *p2 != '\0' )
            p2++;

        if( *p2 == '=' )
        {
            *p2 = '\0';

            if( !strcmp(p1, name) )
            {
                *p2++ = '=';
                *pvalue = simple_strtol(p2, &p1, 0);
                if( *p1 == '\0' )
                    ret = 0;
                break;
            }

            *p2 = '=';
        }

        p1 += strlen(p1) + 1;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: kerSysBlParmsGetStr
 * Description  : Returns the string value for the requested name from
 *                the boot loader parameter buffer.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysBlParmsGetStr( char *name, char *pvalue, int size )
{
    char *p2, *p1 = g_blparms_buf;
    int ret = -1;

    /* The g_blparms_buf buffer contains one or more contiguous NULL termianted
     * strings that ends with an empty string.
     */
    while( *p1 )
    {
        p2 = p1;

        while( *p2 != '=' && *p2 != '\0' )
            p2++;

        if( *p2 == '=' )
        {
            *p2 = '\0';

            if( !strcmp(p1, name) )
            {
                *p2++ = '=';
                strncpy(pvalue, p2, size);
                ret = 0;
                break;
            }

            *p2 = '=';
        }

        p1 += strlen(p1) + 1;
    }

    return( ret );
}

static int proc_get_wan_type(char *buf, char **start, off_t off, int cnt, int *eof, void *data)
{
    int n = 0;

    unsigned long wan_type = 0, t;
    int i, j, len = 0;

    BpGetOpticalWan(&wan_type);
    if (wan_type == BP_NOT_DEFINED)
    {
        sprintf(buf, "none%n", &n);
        return n;
    }

    for (i = 0, j = 0; wan_type; i++)
    {
        t = wan_type & (1 << i);
        if (!t)
            continue;

        wan_type &= ~(1 << i);
        if (j++)
        {
            sprintf(buf + len, "\n");
            len++;
        }

        switch (t)
        {
        case BP_OPTICAL_WAN_GPON:
            sprintf(buf + len, "gpon%n", &n);
            break;
        case BP_OPTICAL_WAN_EPON:
            sprintf(buf + len, "epon%n", &n);
            break;
        case BP_OPTICAL_WAN_AE:
            sprintf(buf + len, "ae%n", &n);
            break;
        default:
            sprintf(buf + len, "unknown%n", &n);
            break;
        }
        len += n;
    }

    return len;
}

static int proc_get_boardid(char *buf, char **start, off_t off, int cnt, int *eof, void *data)
{
    char boardid[NVRAM_BOARD_ID_STRING_LEN];
    kerSysNvRamGetBoardId(boardid);
    sprintf(buf, "%s", boardid);
    return strlen(boardid);
}
#if defined(GPL_CODE_FACTORY_TEST)
static int proc_get_bootimage_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r = 0;
    char partition = '1'; //indicates which image
    *eof = 1;

    partition = getBootedValue(1) == BOOTED_PART1_IMAGE ? '1' : '2';
    r += sprintf(page + r, "%c \n", partition);
    return (r < cnt)? r: 0;
}

static int proc_set_bootimage_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    char input[32];

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;
    if ((cnt > 2) || (copy_from_user(input, buf, cnt) != 0))
        return -EFAULT;

    switch(input[0])
    {
        case '1':
            kerSysSetBootImageState(BOOT_SET_PART1_IMAGE);
            break;
        case '2':
            kerSysSetBootImageState(BOOT_SET_PART2_IMAGE);
            break;
        default:
            printk("error value, must be 1 or 2.\n");
            return -EFAULT;
    }

    return cnt;
}
#endif

#if defined(GPL_CODE_SECURITY_LEVEL_3)
static int proc_get_sensitive_flag(char *buf, char **start, off_t off, int cnt, int *eof, void *data)
{    
    int r = 0;
    NVRAM_DATA *pNvramData = NULL;

    pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_KERNEL);
    if (pNvramData == NULL) {
       return 0;
    }
    if (AEI_readNvramData(pNvramData) == 0) {
	r = snprintf(buf, 9, "%x", pNvramData->sensitive_flag);
    }
    kfree(pNvramData);

    return r;
}
#endif

#if defined(GPL_CODE) && (defined(CONFIG_BCM963138) /*|| defined(CONFIG_BCM963268)*/)
/* BCM reversed these two types(SW_RESET_STATUS and POR_RESET_STATUS) in 63268 */
#if defined(CONFIG_BCM963268)
/* BCM reversed these two types(SW_RESET_STATUS and POR_RESET_STATUS) in 63268 */
#define AEI_POR_RESET_STATUS            SW_RESET_STATUS
#define AEI_WD_RESET_STATUS             POR_RESET_STATUS
#else
#define AEI_POR_RESET_STATUS            POR_RESET_STATUS
#define AEI_WD_RESET_STATUS             SW_RESET_STATUS
unsigned int *swResetStatus = &HS_SPI_PROFILES[7].mode_ctrl;
#endif
#define AEI_HW_RESET_STATUS             HW_RESET_STATUS
#define AEI_SW_NORMAL_RESET_STATUS      SW_NORMAL_RESET_STATUS
#define AEI_SW_EMERGENCY_RESET_STATUS   SW_EMERGENCY_RESET_STATUS
void set_bootstatus(unsigned int status)
{
#if defined(CONFIG_BCM963268)
    TIMER->ClkRstCtl |= status;
#else
    if (status & HS_SPI_FILLBYTE_MASK) {
        /*set bit to 0*/
        *swResetStatus &= ~status;
    } else {
        printk("%s,Wrong status:%x\n", __func__, status);
    }
#endif
}
EXPORT_SYMBOL(set_bootstatus);

static int init_bootstatus(void *data)
{
   unsigned int *bootstatus = (unsigned int *)data;
#if defined(CONFIG_BCM963268)
    *bootstatus = TIMER->ClkRstCtl;
    /* after got the boot status flags, need clear software boot flags */
    TIMER->ClkRstCtl &= ~SW_NORMAL_RESET_STATUS;
    TIMER->ClkRstCtl &= ~SW_EMERGENCY_RESET_STATUS;
#else
    /*read HW reset status*/
    *bootstatus = TIMER->TimerResetStatus & RESET_STATUS_MASK;
    /*read SW reset status*/
    *bootstatus |= (~*swResetStatus) & HS_SPI_FILLBYTE_MASK;
    HS_SPI->hs_spiGlobalCtrl |= HS_SPI_DO_NOT_RESET_ON_WATCHDOG;
    /*clear SW reset status, the HS_SPI_FILLBYTE reset value is 0xff*/
    *swResetStatus |= HS_SPI_FILLBYTE_MASK;
#endif
    return 0;
}

static int proc_get_bootstatus(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r = 0;
    unsigned int bootstatus = *(unsigned int *)data;

    r += sprintf(page + r, "%s", "boot by: ");
    if (bootstatus & AEI_POR_RESET_STATUS) { /* power on */
        r += sprintf(page + r, "%s \n", "POWER_RESET");
    } else if (bootstatus & AEI_SW_EMERGENCY_RESET_STATUS) { /* emergency reboot: Kernel panic, etc */
        r += sprintf(page + r, "%s \n", "EMERGENCY_RESET");
    } else if (bootstatus & AEI_SW_NORMAL_RESET_STATUS) { /* normal reboot: issue reboot, hold button do reset, etc */
        r += sprintf(page + r, "%s \n", "NORMAL_RESET");
    } else if (bootstatus & AEI_WD_RESET_STATUS) { /* watchdog reset */
        r += sprintf(page + r, "%s \n", "WATCHDOG_RESET");
    } else if (bootstatus & AEI_HW_RESET_STATUS) { /* hardware reset, but we don't understand this type reset */
        r += sprintf(page + r, "%s \n", "HARDWARE_RESET");
    }

    return (r < cnt)? r: 0;
}
#endif

static int add_proc_files(void)
{
#define offset(type, elem) ((int)&((type *)0)->elem)

    static int BaseMacAddr[2] = {offset(NVRAM_DATA, ucaBaseMacAddr), NVRAM_MAC_ADDRESS_LEN};
#if defined(GPL_CODE_DUAL_IMAGE) || defined(GPL_CODE_VER2_DUAL_IMAGE) || defined(GPL_CODE_FACTORY_TEST)
    static int BootImage[2] = {offset(NVRAM_DATA, szBootline), NVRAM_BOOTLINE_LEN};
#endif

#if defined(GPL_CODE)
    static int Serial_num[2] = {offset(NVRAM_DATA, ulSerialNumber), 32};
    static int Wpa_key[2] = {offset(NVRAM_DATA, wpaKey), 32};
    static int Wps_pin[2] = {offset(NVRAM_DATA,wpsPin), 32};
    static int Hw_ver[2] = {offset(NVRAM_DATA, chFactoryFWVersion), 48};
#endif
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT)   
    static int Wlan_feature[2] = {offset(NVRAM_DATA,wlanParams[NVRAM_WLAN_PARAMS_LEN -1]) , 1};
#endif
    struct proc_dir_entry *p0;
    struct proc_dir_entry *p1;
    struct proc_dir_entry *p2;
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    struct proc_dir_entry *p3;
#endif
    struct proc_dir_entry *p4;
#if defined(GPL_CODE) && (defined(CONFIG_BCM963138) || defined(CONFIG_BCM963268))
    static unsigned int bootstatus = 0;
#endif

    p0 = proc_mkdir("nvram", NULL);

    if (p0 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }
#if defined(GPL_CODE_FACTORY_TEST)
    p1 = create_proc_entry("BootImage", 0644, p0); 

    if (p1 == NULL)
    {    
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }    

    p1->data        = BootImage;
    p1->read_proc   = proc_get_bootimage_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_bootimage_param;
#endif
#endif
#if defined(GPL_CODE_24G_WIFI_CALIBRATION_TEST_SUPPORT) 
    p1 = create_proc_entry("Wlan_feature", 0644, p0);
    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }
            
    p1->data        = Wlan_feature;
    p1->read_proc   = proc_get_wlan_feature_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_wlan_feature_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif
#endif
    p1 = create_proc_entry("BaseMacAddr", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = BaseMacAddr;
    p1->read_proc   = proc_get_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

#if defined(GPL_CODE_DUAL_IMAGE) || defined(GPL_CODE_VER2_DUAL_IMAGE)
    p1 = create_proc_entry("BootImage", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = BootImage;
    p1->read_proc   = proc_get_bootimage_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_bootimage_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

#endif

#if defined(GPL_CODE)
    p1 = create_proc_entry("Serial_num", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = Serial_num;
    p1->read_proc   = proc_get_other_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_other_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

    p1 = create_proc_entry("Wpa_key", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = Wpa_key;
    p1->read_proc   = proc_get_other_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_other_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif
 p1 = create_proc_entry("Wps_pin", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = Wps_pin;
    p1->read_proc   = proc_get_other_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_other_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

    p1 = create_proc_entry("Hw_ver", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = Hw_ver;
    p1->read_proc   = proc_get_other_param;
#if !defined(GPL_CODE_PROHIBIT_MODIFY_NVRAM)
    p1->write_proc  = proc_set_other_param;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

#if 0
    p1 = create_proc_entry("Ssid", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = Ssid;
    p1->read_proc   = proc_get_other_param;
    p1->write_proc  = proc_set_other_param;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif
#endif
#endif
    p1 = create_proc_entry("led", 0644, NULL);
    if (p1 == NULL)
        return -1;

    p1->write_proc  = proc_set_led;

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
    p3 = create_proc_entry("show_rdp_mem", 0444, NULL);
    if (p3 == NULL)
        return -1;

    p3->read_proc  = proc_show_rdp_mem;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

    p2 = create_proc_entry("supported_optical_wan_types", 0444, p0);
    if (p2 == NULL)
        return -1;
    p2->read_proc = proc_get_wan_type;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p2->owner       = THIS_MODULE;
#endif

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
    p2 = create_proc_entry("watchdog", 0644, NULL);
    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create watchdog proc file!\n");
        return -1;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p2->owner       = THIS_MODULE;
#endif

    p2->data        = NULL;
    p2->read_proc   = proc_get_watchdog;
    p2->write_proc  = proc_set_watchdog;
#endif /* defined(CONFIG_BCM_WATCHDOG_TIMER) */

    p4 = create_proc_entry("boardid", 0444, p0);
    if (p4 == NULL)
        return -1;
    p4->read_proc = proc_get_boardid;
#if defined(GPL_CODE_SECURITY_LEVEL_3)
    p4 = create_proc_entry("sensitive_flag", 0444, p0);
    if (p4 == NULL) {
        printk("add_proc_files: failed to create sensitive_flag proc file!\n");
        return -1;
    }
    p4->read_proc = proc_get_sensitive_flag;
#endif

#if defined(GPL_CODE_DETECT_BOARD_ID)
    /* create /proc/hwBoardId file  */
    p1 = create_proc_entry("hwBoardId", 0644, &proc_root);
    if (p1 == NULL){
        printk("add_proc_files: failed to create proc files!\n");
    }  
    p1->read_proc = boardid_proc_read;
#endif
#if defined(GPL_CODE) && (defined(CONFIG_BCM963138) /* || defined(CONFIG_BCM963268)*/)
    p1 = create_proc_entry("bootstatus", 0444, NULL);
    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create bootstatus proc files!\n");
        return -1;
    }
    init_bootstatus(&bootstatus);
    p1->data        = &bootstatus;
    p1->read_proc   = proc_get_bootstatus;
#endif
    return 0;
}

static int del_proc_files(void)
{
    remove_proc_entry("nvram", NULL);
    remove_proc_entry("led", NULL);
    return 0;
}

// Use this ONLY to convert strings of bytes to strings of chars
// use functions from linux/kernel.h for everything else
static void str_to_num(char* in, char* out, int len)
{
    int i;
    memset(out, 0, len);

    for (i = 0; i < len * 2; i ++)
    {
        if ((*in >= '0') && (*in <= '9'))
            *out += (*in - '0');
        else if ((*in >= 'a') && (*in <= 'f'))
            *out += (*in - 'a') + 10;
        else if ((*in >= 'A') && (*in <= 'F'))
            *out += (*in - 'A') + 10;
        else
            *out += 0;

        if ((i % 2) == 0)
            *out *= 16;
        else
            out ++;

        in ++;
    }
    return;
}

static int proc_get_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int i = 0;
    int r = 0;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    NVRAM_DATA *pNvramData;

    *eof = 1;
    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    if (NULL != (pNvramData = readNvramData()))
    {
        for (i = 0; i < length; i ++)
            r += sprintf(page + r, "%02x ", ((unsigned char *)pNvramData)[offset + i]);
    }

    r += sprintf(page + r, "\n");
    if (pNvramData)
        kfree(pNvramData);
    return (r < cnt)? r: 0;
}

static int proc_set_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    NVRAM_DATA *pNvramData;
    char input[32];

    int i = 0;
    int r = cnt;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    if ((cnt > 32) || (copy_from_user(input, buf, cnt) != 0))
        return -EFAULT;

    for (i = 0; i < r; i ++)
    {
        if (!isxdigit(input[i]))
        {
            memmove(&input[i], &input[i + 1], r - i - 1);
            r --;
            i --;
        }
    }

    mutex_lock(&flashImageMutex);

    if (NULL != (pNvramData = readNvramData()))
    {
        str_to_num(input, ((char *)pNvramData) + offset, length);
        writeNvramDataCrcLocked(pNvramData);
    }
    else
    {
        cnt = 0;
    }

    mutex_unlock(&flashImageMutex);

    if (pNvramData)
        kfree(pNvramData);

    return cnt;
}

/*
 * This function expect input in the form of:
 * echo "xxyy" > /proc/led
 * where xx is hex for the led number
 * and   yy is hex for the led state.
 * For example,
 *     echo "0301" > led
 * will turn on led 3
 */
static int proc_set_led(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    // char leddata[16];
    unsigned int leddata;
    char input[32];
    int i;

    if (cnt > 31)
        cnt = 31;

    if (copy_from_user(input, buf, cnt) != 0)
        return -EFAULT;


    for (i = 0; i < cnt; i ++)
    {
        if (!isxdigit(input[i]))
        {
            input[i] = 0;
        }
    }
    input[i] = 0;

    if (0 != kstrtouint(input, 16, &leddata)) 
        return -EFAULT;

    kerSysLedCtrl ((leddata & 0xff00) >> 8, leddata & 0xff);
    return cnt;
}

static void start_watchdog(unsigned int timer, unsigned int reset) 
{
#if defined(CONFIG_BCM_WATCHDOG_TIMER)
    unsigned long flags;

    spin_lock_irqsave(&watchdog_spinlock, flags); 

    /* if watch dog is disabled and reset is 0, do nothing */
    if (!reset && !watchdog_data.enabled)
    {
        spin_unlock_irqrestore(&watchdog_spinlock, flags); 
        return;
    }
#endif /* defined(CONFIG_BCM_WATCHDOG_TIMER) */

#if defined (CONFIG_BCM96838)
    WDTIMER->WD0Ctl = 0xEE00;
    WDTIMER->WD0Ctl = 0x00EE;
    WDTIMER->WD0DefCount = timer * (FPERIPH/1000000);
    WDTIMER->WD0Ctl = 0xFF00;
    WDTIMER->WD0Ctl = 0x00FF;
#else
    TIMER->WatchDogCtl = 0xEE00;
    TIMER->WatchDogCtl = 0x00EE;
    TIMER->WatchDogDefCount = timer * (FPERIPH/1000000);
    TIMER->WatchDogCtl = 0xFF00;
    TIMER->WatchDogCtl = 0x00FF;
#endif

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
    /* when reset is 1, disable interrupt */
    if (reset && watchdog_data.enabled)
    {
#if defined(INTERRUPT_ID_WDTIMER)
        BcmHalInterruptDisable(INTERRUPT_ID_WDTIMER);
#elif defined(CONFIG_BCM_EXT_TIMER)
        watchdog_callback_register(NULL);
#else
        BcmHalInterruptDisable(INTERRUPT_ID_TIMER);
#endif
    }
    else
    {
        /* reset userTimeout value */
        if (watchdog_data.userMode)
            watchdog_data.userTimeout = 0;
    }
    spin_unlock_irqrestore(&watchdog_spinlock, flags); 
#endif /* defined(CONFIG_BCM_WATCHDOG_TIMER)*/
}

#if defined(CONFIG_BCM_WATCHDOG_TIMER)
int bcm_suspend_watchdog() 
{
    unsigned long flags;
    int needResume = 0;

    spin_lock_irqsave(&watchdog_spinlock, flags); 

    if (watchdog_data.enabled && !watchdog_data.suspend)
    {
#if defined (CONFIG_BCM96838)
        WDTIMER->WD0Ctl = 0xEE00;
        WDTIMER->WD0Ctl = 0x00EE;
#else
        TIMER->WatchDogCtl = 0xEE00;
        TIMER->WatchDogCtl = 0x00EE;
#endif

        watchdog_data.suspend = 1;
        needResume = 1;
    } 

    spin_unlock_irqrestore(&watchdog_spinlock, flags); 
    return needResume;
}

void bcm_resume_watchdog() 
{
    unsigned long flags;

    start_watchdog(watchdog_data.timer, 0);

    spin_lock_irqsave(&watchdog_spinlock, flags); 
    watchdog_data.suspend = 0;
    spin_unlock_irqrestore(&watchdog_spinlock, flags); 
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
static void watchdog_restart_thread(struct work_struct *work)
#else
static void watchdog_restart_thread(void *arg)
#endif
{
    // kernel_restart is a high level, generic linux way of rebooting.
    // It calls a notifier list and lets sub-systems know that system is
    // rebooting, and then calls machine_restart, which eventually
    // calls kerSysMipsSoftReset.
    kernel_restart(NULL);

    return;
}

#if defined(INTERRUPT_ID_WDTIMER) || !defined(CONFIG_BCM_EXT_TIMER)
static irqreturn_t watchdog_isr(int irq, void *ignore)
#else
static void watchdog_isr(int param)
#endif
{
    unsigned long flags;
    struct list_head *pos;
    CB_WDOG_LIST *tmp = NULL;
    int reboot = 0;

#if !defined(INTERRUPT_ID_WDTIMER) && !defined(CONFIG_BCM_EXT_TIMER)
    /* 
     * if WD shares timer interrupt and EXT_TIMER is disabled, 
     * need to check if it is WD interrupt
     */ 
    if (!(TIMER->TimerInts & WATCHDOG))
        return IRQ_NONE;
#endif

    spin_lock_irqsave(&watchdog_spinlock, flags); 

#if defined(INTERRUPT_ID_WDTIMER) || !defined(CONFIG_BCM_EXT_TIMER)
    /* 
     * if WD shares timer interrupt and EXT_TIMER is enabled, 
     * interrupt be cleared in ext_timer_isr.
     */ 

    /* clear the interrupt */
    TIMER->TimerInts |= WATCHDOG;

#if !defined(CONFIG_ARM)
#if defined(INTERRUPT_ID_WDTIMER)
    BcmHalInterruptEnable(INTERRUPT_ID_WDTIMER);
#else
    BcmHalInterruptEnable(INTERRUPT_ID_TIMER);
#endif /* defined(INTERRUPT_ID_WDTIMER) */
#endif /* !defined(CONFIG_ARM) */
#endif /*  defined(INTERRUPT_ID_WDTIMER) || !defined(CONFIG_BCM_EXT_TIMER) */

#if defined (CONFIG_BCM96838)
    WDTIMER->WD0Ctl = 0xEE00;
    WDTIMER->WD0Ctl = 0x00EE;
    WDTIMER->WD0Ctl = 0xFF00;
    WDTIMER->WD0Ctl = 0x00FF;
#else
    /* stop and reload timer counter then start WD */
    TIMER->WatchDogCtl = 0xEE00;
    TIMER->WatchDogCtl = 0x00EE;
    TIMER->WatchDogCtl = 0xFF00;
    TIMER->WatchDogCtl = 0x00FF;
#endif

    /* check watchdog callback function */
    list_for_each(pos, &g_cb_wdog_list_head->list) 
    {
        tmp = list_entry(pos, CB_WDOG_LIST, list);
        if ((tmp->cb_wd_fn)(tmp->context))
        {
            reboot = 1;
            printk("\nwatchdog cb of %s return 1, reset CPE!!!\n", tmp->name);
            break;
        }
    }

    if (!reboot && watchdog_data.userMode)
    {
        watchdog_data.userTimeout++;
        if (watchdog_data.userTimeout >= watchdog_data.userThreshold)
        {
            reboot = 1;
            printk("\nHit userMode watchdog threshold, reset CPE!!!\n");
        }
    }

    if (reboot)
    {
        spin_unlock_irqrestore(&watchdog_spinlock, flags); 
        bcm_suspend_watchdog();
        /* 
         *  If call kerSysMipsSoftReset() in interrupt,  
         *  kernel smp pops out warning.  
         */
        if( !watchdog_restart_in_progress )
        {
            INIT_WORK(&watchdogRestartWork, watchdog_restart_thread);
            schedule_work(&watchdogRestartWork);
            watchdog_restart_in_progress  = 1;
        }

#if defined(INTERRUPT_ID_WDTIMER) || !defined(CONFIG_BCM_EXT_TIMER)
        return IRQ_HANDLED;
#else
        return;
#endif
    }

    spin_unlock_irqrestore(&watchdog_spinlock, flags); 

#if defined(INTERRUPT_ID_WDTIMER) || !defined(CONFIG_BCM_EXT_TIMER)
    return IRQ_HANDLED;
#endif
}

static int proc_get_watchdog(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{

    int r = 0;
    *eof = 1;
    r += sprintf(page + r, "watchdog enabled=%u timer=%u ns suspend=%u\n", 
                           watchdog_data.enabled, 
                           watchdog_data.timer, 
                           watchdog_data.suspend);
    r += sprintf(page + r, "         userMode=%u userThreshold=%u userTimeout=%u\n", 
                           watchdog_data.userMode, 
                           watchdog_data.userThreshold/2, 
                           watchdog_data.userTimeout/2);
    return (r < cnt)? r: 0;
}

static int proc_set_watchdog(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    unsigned long flags;
    char input[64];
    unsigned int enabled, timer;
    unsigned int userMode, userThreshold;
   
    if (cnt > 64) 
    {
        cnt = 64;
    }

    if (copy_from_user(input, buf, cnt) != 0) 
    {
        return -EFAULT;
    }

    if (strncasecmp(input, "OK", 2) == 0)
    {
        spin_lock_irqsave(&watchdog_spinlock, flags); 
        if (watchdog_data.userMode)
            watchdog_data.userTimeout = 0;
        spin_unlock_irqrestore(&watchdog_spinlock, flags); 
        return cnt;
    }

    if (sscanf(input, "%u %u %u %u", &enabled, &timer, &userMode, &userThreshold) != 4)
    {
        printk("\nError format, it is as:\n");
        printk("\"enabled(0|1) timer(ns) userMode(0|1) userThreshold\"\n");
        return -EFAULT;
    }

    spin_lock_irqsave(&watchdog_spinlock, flags); 
    
    watchdog_data.userMode = userMode;
    watchdog_data.userThreshold = userThreshold * 2; // watchdog interrupt is half of timer
    watchdog_data.userTimeout = 0;             // reset userTimeout
    if (watchdog_data.enabled != enabled) 
    { 
        watchdog_data.timer = timer;
        if (enabled)
        {
#if defined(INTERRUPT_ID_WDTIMER)
            BcmHalMapInterrupt((FN_HANDLER)watchdog_isr , 0, INTERRUPT_ID_WDTIMER);
#if !defined(CONFIG_ARM)
            BcmHalInterruptEnable(INTERRUPT_ID_WDTIMER);
#endif /* !defined(CONFIG_ARM) */
#elif defined(CONFIG_BCM_EXT_TIMER)
            watchdog_callback_register(&watchdog_isr);
#else
            /* 
             *  The 2nd parameter must be unique to share same IRQ.
             *  We need to pass the same magic value when call free_irq().
             */
            BcmHalMapInterrupt((FN_HANDLER)watchdog_isr, 0xabcd1212, INTERRUPT_ID_TIMER);
            BcmHalInterruptEnable(INTERRUPT_ID_TIMER);
#endif
            watchdog_data.enabled = enabled;
            watchdog_data.suspend = 0;
            spin_unlock_irqrestore(&watchdog_spinlock, flags); 
            start_watchdog(watchdog_data.timer, 0);
        }
        else
        {
            spin_unlock_irqrestore(&watchdog_spinlock, flags); 
            bcm_suspend_watchdog();
#if defined(INTERRUPT_ID_WDTIMER)
            free_irq(INTERRUPT_ID_WDTIMER, NULL);
#elif defined(CONFIG_BCM_EXT_TIMER)
            watchdog_callback_register(NULL);
#else
            free_irq(INTERRUPT_ID_TIMER, (void *)0xabcd1212);
#endif
            watchdog_data.enabled = enabled;
        }
    }
    else if (watchdog_data.timer != timer)
    {
        watchdog_data.timer = timer;
        if (watchdog_data.enabled)
        {
            watchdog_data.suspend = 0;
            spin_unlock_irqrestore(&watchdog_spinlock, flags); 
            start_watchdog(watchdog_data.timer, 0);
        }    
    }
    else
        spin_unlock_irqrestore(&watchdog_spinlock, flags); 

    return cnt;
}

static void __init kerSysInitWatchdogCBList( void )
{
    CB_WDOG_LIST *new_node;
    unsigned long flags;

    if( g_cb_wdog_list_head != NULL) 
    {
        printk("Error: kerSysInitWatchdogCBList: list head is not null\n");
        return;
    }
    new_node= (CB_WDOG_LIST *)kmalloc(sizeof(CB_WDOG_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_WDOG_LIST));
    INIT_LIST_HEAD(&new_node->list);
    spin_lock_irqsave(&watchdog_spinlock, flags); 
    g_cb_wdog_list_head = new_node;
    spin_unlock_irqrestore(&watchdog_spinlock, flags); 
} 

void kerSysRegisterWatchdogCB(char *devname, void *cbfn, void *context)
{
    CB_WDOG_LIST *new_node;
    unsigned long flags;

    // do all the stuff that can be done without the lock first
    if( devname == NULL || cbfn == NULL ) 
    {
        printk("Error: kerSysRegisterWatchdogCB: register info not enough (%s,%x,%x)\n", devname, (unsigned int)cbfn, (unsigned int)context);
        return;
    }

    if (strlen(devname) > (IFNAMSIZ - 1))
        printk("Warning: kerSysRegisterWatchdogCB: devname too long, will be truncated\n");

    new_node= (CB_WDOG_LIST *)kmalloc(sizeof(CB_WDOG_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_WDOG_LIST));
    INIT_LIST_HEAD(&new_node->list);
    strncpy(new_node->name, devname, IFNAMSIZ-1);
    new_node->cb_wd_fn = (cb_watchdog_t)cbfn;
    new_node->context = context;

    // OK, now acquire the lock and insert into list
    spin_lock_irqsave(&watchdog_spinlock, flags); 
    if( g_cb_wdog_list_head == NULL) 
    {
        printk("Error: kerSysRegisterWatchdogCB: list head is null\n");
        kfree(new_node);
    } 
    else 
    {
        list_add(&new_node->list, &g_cb_wdog_list_head->list);
        printk("watchdog: kerSysRegisterWatchdogCB: %s registered \n", devname);
    }
    spin_unlock_irqrestore(&watchdog_spinlock, flags); 

    return;
} 

void kerSysDeregisterWatchdogCB(char *devname)
{
    struct list_head *pos;
    CB_WDOG_LIST *tmp;
    int found=0;
    unsigned long flags;

    if(devname == NULL) {
        printk("Error: kerSysDeregisterWatchdogCB: devname is null\n");
        return;
    }

    printk("kerSysDeregisterWatchdogCB: %s is deregistering\n", devname);

    spin_lock_irqsave(&watchdog_spinlock, flags); 
    if(g_cb_wdog_list_head == NULL) 
    {
        printk("Error: kerSysDeregisterWatchdogCB: list head is null\n");
    } 
    else 
    {
        list_for_each(pos, &g_cb_wdog_list_head->list) 
        {
            tmp = list_entry(pos, CB_WDOG_LIST, list);
            if(!strcmp(tmp->name, devname)) 
            {
                list_del(pos);
                kfree(tmp);
                found = 1;
                printk("kerSysDeregisterWatchdogCB: %s is deregistered\n", devname);
                break;
            }
        }
        if (!found)
            printk("kerSysDeregisterWatchdogCB: %s not (de)registered\n", devname);
    }
    spin_unlock_irqrestore(&watchdog_spinlock, flags); 

    return;
} 

#endif /* defined(CONFIG_BCM_WATCHDOG_TIMER) */

static DEFINE_SPINLOCK(pinmux_spinlock);

void kerSysInitPinmuxInterface(unsigned long interface) {
    unsigned long flags;
    spin_lock_irqsave(&pinmux_spinlock, flags); 
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
    bcm_init_pinmux_interface(interface);
#endif
    spin_unlock_irqrestore(&pinmux_spinlock, flags); 
}



/***************************************************************************
 * Function Name: kerSysGetUbusFreq
 * Description  : Chip specific computation.
 * Returns      : the UBUS frequency value in MHz.
 ***************************************************************************/
unsigned long kerSysGetUbusFreq(unsigned long miscStrapBus)
{
   unsigned long ubus = UBUS_BASE_FREQUENCY_IN_MHZ;

#if defined(CONFIG_BCM96362)
   /* Ref RDB - 6362 */
   switch (miscStrapBus) {

      case 0x4 :
      case 0xc :
      case 0x14:
      case 0x1c:
      case 0x15:
      case 0x1d:
         ubus = 100;
         break;
      case 0x2 :
      case 0xa :
      case 0x12:
      case 0x1a:
         ubus = 96;
         break;
      case 0x1 :
      case 0x9 :
      case 0x11:
      case 0xe :
      case 0x16:
      case 0x1e:
         ubus = 200;
         break;
      case 0x6:
         ubus = 183;
         break;
      case 0x1f:
         ubus = 167;
         break;
      default:
         ubus = 160;
         break;
   }
#endif

   return (ubus);

}  /* kerSysGetUbusFreq */

#if defined(GPL_CODE)
int AEI_getCurrentUpWanType()
{
    return aeiWanType;
}
#endif

#ifdef GPL_CODE
void restoreDatapump(int value){
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("%s could not read nvram data\n",__FUNCTION__);
        return;
    }

    pNvramData->dslDatapump=value;
    writeNvramDataCrcLocked(pNvramData);
}

int kerSysGetDslDatapump(unsigned char *dslDatapump)
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
        printk("%s could not read nvram data\n",__FUNCTION__);
    else
        *dslDatapump = (unsigned char)pNvramData->dslDatapump;
    return 0;
}
#endif

#ifdef GPL_CODE_TWO_IN_ONE_FIRMWARE
int kerSysGetBoardID(unsigned char *boardid)
{
    NVRAM_DATA *pNvramData;
    if(boardid==NULL)
        return -1;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("%s could not read nvram data\n",__FUNCTION__);
        return -1;
    }
    else
        strcpy(boardid,pNvramData->szBoardId);
    return 0;
}

#endif
#if defined(GPL_CODE_DOWNGRADE_NVRAM_ADJUST)

/*
 * Function Name: AEI_IsDownGrade_FromSDK12To6
 * Description  : Check whether it is downgrade form sdk1204 to sdk6 firmware.
 *                sdk6 firmware's version number is between 31.60L.0c and 31.60L.18,if firmware's minor number
 *                is "60", we will deal with it as downgrade from sdk1204 to sdk6.Maybe there's more effectvie
 *                ways to check it.
 *input         : buffer of firmware needed to write to flash.
 *return        : downgrading will return TRUE,otherwise will return FALSE
 */
       #define  BUFL            10
#if defined(GPL_CODE_SASKTEL)
       #define  SDK6_BUILD_NMMBER  4
#else
       #define  SDK6_MINOR_NUMBER  "60L"
#endif
static UBOOL8 AEI_IsDownGrade_FromSDK12To6(char* string)
{
    PFILE_TAG pTag=NULL;
    char major[BUFL]={0};
    char minor[BUFL]={0};
    char build[BUFL]={0};
    if(string ==NULL)
    {
       printk("AEI_IsDownGrade_FromSDK12To6:Invalid string\n");
       return FALSE;
    }
    pTag=(PFILE_TAG)string;
    if(pTag->signiture_2 != NULL && (strlen(pTag->signiture_2) < SIG_LEN_2))
    {
       if(sscanf(pTag->signiture_2,"%2s.%3s.%s",major,minor,build))
       {
#if defined(GPL_CODE_SASKTEL)
          int mbuild=0;
          sscanf(build,"%d",&mbuild);
          if(mbuild <= SDK6_BUILD_NMMBER)
#else
          if( strncmp(minor,SDK6_MINOR_NUMBER,3)==0 )
#endif
            return TRUE;
       }
    }
    return FALSE;
}

#if defined(GPL_CODE_DOWNGRADE_NVRAM_ADJUST)
/* Function Name :AEI_DownGrade_AdjustNVRAM
 * Description   :Adjust NVRAM mapping when downgarde form sdk1204 to sdk6 firmware.
 */
static UBOOL8 AEI_DownGrade_AdjustNVRAM(void)
{

    NVRAM_DATA nvramData;
    TELUS_V2000H_NVRAM_DATA telus_v2000h_nvramData;
    AEI_readNvramData(&nvramData);
    AEI_readNvramData((PNVRAM_DATA)&telus_v2000h_nvramData);
    if(nvramData.ulVersion == 6
         && nvramData.ulSerialNumber[0] != '\0'
         && nvramData.chFactoryFWVersion[0] != '\0'
         && (unsigned char)nvramData.ulSerialNumber[0] != 0xFF
         && (unsigned char)nvramData.chFactoryFWVersion[0] != 0xFF
         && strncmp(nvramData.ulSerialNumber,telus_v2000h_nvramData.ulSerialNumber,32)!=0)
    {
        memset(&telus_v2000h_nvramData, 0, sizeof(telus_v2000h_nvramData));

        memcpy(&telus_v2000h_nvramData.szBootline[0],&nvramData.szBootline[0],NVRAM_BOOTLINE_LEN);
        memcpy(&telus_v2000h_nvramData.szBoardId[0],&nvramData.szBoardId[0],NVRAM_BOARD_ID_STRING_LEN);
        telus_v2000h_nvramData.ulMainTpNum = nvramData.ulMainTpNum;
        telus_v2000h_nvramData.ulPsiSize = nvramData.ulPsiSize;
        telus_v2000h_nvramData.ulNumMacAddrs = nvramData.ulNumMacAddrs;
        memcpy(&telus_v2000h_nvramData.ucaBaseMacAddr[0],&nvramData.ucaBaseMacAddr[0],NVRAM_MAC_ADDRESS_LEN);

        telus_v2000h_nvramData.pad = nvramData.pad;
        telus_v2000h_nvramData.backupPsi = nvramData.backupPsi;
        telus_v2000h_nvramData.ulCheckSumV4 = nvramData.ulCheckSumV4;

        memcpy(&telus_v2000h_nvramData.gponSerialNumber[0],&nvramData.gponSerialNumber[0],NVRAM_GPON_SERIAL_NUMBER_LEN);
        memcpy(&telus_v2000h_nvramData.gponPassword[0],&nvramData.gponPassword[0],NVRAM_GPON_PASSWORD_LEN);
        memcpy(&telus_v2000h_nvramData.wpsDevicePin[0],&nvramData.wpsDevicePin[0],NVRAM_WPS_DEVICE_PIN_LEN);
        memcpy(&telus_v2000h_nvramData.wlanParams[0],&nvramData.wlanParams[0],NVRAM_WLAN_PARAMS_LEN);

        telus_v2000h_nvramData.ulSyslogSize = nvramData.ulSyslogSize;

        memcpy(&telus_v2000h_nvramData.ulNandPartOfsKb[0], &nvramData.ulNandPartOfsKb[0], sizeof(nvramData.ulNandPartOfsKb));
        memcpy(&telus_v2000h_nvramData.ulNandPartSizeKb[0], &nvramData.ulNandPartSizeKb[0], sizeof(nvramData.ulNandPartSizeKb));
        memcpy(&telus_v2000h_nvramData.szVoiceBoardId[0], &nvramData.szVoiceBoardId[0], sizeof(nvramData.szVoiceBoardId));

        memcpy(&telus_v2000h_nvramData.afeId[0], &nvramData.afeId[0], sizeof(nvramData.afeId));

        memcpy(&telus_v2000h_nvramData.ulSerialNumber[0],&nvramData.ulSerialNumber[0],32);
        memcpy(&telus_v2000h_nvramData.chFactoryFWVersion[0],&nvramData.chFactoryFWVersion[0],48);
        memcpy(&telus_v2000h_nvramData.wpsPin[0],&nvramData.wpsPin[0],32);
        memcpy(&telus_v2000h_nvramData.wpaKey[0],&nvramData.wpaKey[0],32);
        telus_v2000h_nvramData.dslDatapump=nvramData.dslDatapump;

        telus_v2000h_nvramData.ulVersion=NVRAM_VERSION_NUMBER;

        memset((char *)&telus_v2000h_nvramData + ((size_t) &((NVRAM_DATA *)0)->ulSerialNumber), 0, 1);
        memset((char *)&telus_v2000h_nvramData + ((size_t) &((NVRAM_DATA *)0)->chFactoryFWVersion), 0, 1);

        telus_v2000h_nvramData.ulCheckSum = getCrc32((unsigned char *)&telus_v2000h_nvramData, sizeof(TELUS_V2000H_NVRAM_DATA),CRC32_INIT_VALUE);

        writeNvramDataCrcLocked(&telus_v2000h_nvramData);
        return TRUE;
     }
     return FALSE;

}
#endif
#endif

/***************************************************************************
 * Function Name: kerSysGetChipId
 * Description  : Map id read from device hardware to id of chip family
 *                consistent with  BRCM_CHIP
 * Returns      : chip id of chip family
 ***************************************************************************/
int kerSysGetChipId() { 
        int r;
#if   defined(CONFIG_BCM96838)
        r = 0x6838;
#elif defined(CONFIG_BCM96848)
        r = 0x6848;
#elif defined(CONFIG_BCM960333)
        r = 0x60333;
#else
        r = (int) ((PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT);
        /* Force BCM63168, BCM63169, and BCM63269 to be BCM63268) */
        if( ( (r & 0xffffe) == 0x63168 )
          || ( (r & 0xffffe) == 0x63268 ))
            r = 0x63268;

        /* Force 6319 to be BCM6318 */
        if (r == 0x6319)
            r = 0x6318;

#endif

        return(r);
}

#if !defined(CONFIG_BCM960333)
/***************************************************************************
 * Function Name: kerSysGetDslPhyEnable
 * Description  : returns true if device should permit Phy to load
 * Returns      : true/false
 ***************************************************************************/
int kerSysGetDslPhyEnable() {
        int id;
        int r = 1;
        id = (int) ((PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT);
        if ((id == 0x63169) || (id == 0x63269)) {
	    r = 0;
        }
        return(r);
}
#endif

/***************************************************************************
 * Function Name: kerSysGetChipName
 * Description  : fills buf with the human-readable name of the device
 * Returns      : pointer to buf
 ***************************************************************************/
char *kerSysGetChipName(char *buf, int n) {
	return(UtilGetChipName(buf, n));
}

/***************************************************************************
 * Function Name: kerSysGetExtIntInfo
 * Description  : return the external interrupt information which includes the
 *                trigger type, sharing enable.
 * Returns      : pointer to buf
 ***************************************************************************/
unsigned int kerSysGetExtIntInfo(unsigned int irq)
{
	return extIntrInfo[irq-INTERRUPT_ID_EXTERNAL_0];
}


int kerSysGetPciePortEnable(int port)
{
	int ret = 1;
#if defined (CONFIG_BCM96838)
	unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;

    switch (chipId)
	{
		case 1:		// 68380
		case 6:		// 68380M
		case 7:		// 68389
			ret = 1;
			break;
			
		case 3:		// 68380F
			if(port == 0)
				ret = 1;
			else
				ret = 0;
			break;
		
		case 4:		// 68385
		case 5:		// 68381
		default:
			ret = 0;
			break;
    }
#elif defined (CONFIG_BCM96848)
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;

    switch (chipId)
    {
        case 0x050d:    // 68480F
        case 0x051a:    // 68481P
        case 0x05c0:    // 68486
            ret = 1;
            break;
        default:
            ret = 0;    // 68485, 68481
            break;
    }

    if (port != 0)
        ret = 0;
#endif	
	return ret;
}
EXPORT_SYMBOL(kerSysGetPciePortEnable);

int kerSysGetUsbHostPortEnable(int port)
{
	int ret = 1;
#if defined (CONFIG_BCM96838)
	unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
	
    switch (chipId)
	{
		case 1:		// 68380
		case 6:		// 68380M
		case 7:		// 68389
			ret = 1;
			break;
			
		case 3:		// 68380F
			if(port == 0)
				ret = 1;
			else
				ret = 0;
			break;
		
		case 4:		// 68385
		case 5:		// 68381
		default:
			ret = 0;
			break;
    }
#elif defined (CONFIG_BCM96848)
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;

    switch (chipId)
    {
        case 0x050d:    // 68480F
        case 0x05c0:    // 68486
            ret = 1;
            break;
        default:
            ret = 0;
            break;
    }

	if(port != 0)
		ret = 0;
#endif	
	return ret;
}
EXPORT_SYMBOL(kerSysGetUsbHostPortEnable);

int kerSysGetUsbDeviceEnable(void)
{
	int ret = 1;
	
#if defined (CONFIG_BCM96838) || defined (CONFIG_BCM96848)
	ret = 0;
#endif	

	return ret;
}
EXPORT_SYMBOL(kerSysGetUsbDeviceEnable);

int kerSysGetUsb30HostEnable(void)
{
	int ret = 0;
	
#if defined(CONFIG_BCM963138)|| defined(CONFIG_BCM963148)
	ret = 1;
#endif	

	return ret;
}
EXPORT_SYMBOL(kerSysGetUsb30HostEnable);

int kerSysSetUsbPower(int on, USB_FUNCTION func)
{
	int status = 0;
#if !defined(CONFIG_BRCM_IKOS)
#if defined (CONFIG_BCM96838)
	static int usbHostPwr = 1;
	static int usbDevPwr = 1;
	
	if(on)
	{
		if(!usbHostPwr && !usbDevPwr)
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_Common);
		
		if(((func == USB_HOST_FUNC) || (func == USB_ALL_FUNC)) && !usbHostPwr && !status)
		{
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Host);
			if(!status)
				usbHostPwr = 1;
		}
		
		if(((func == USB_DEVICE_FUNC) || (func == USB_ALL_FUNC)) && !usbDevPwr && !status)
		{
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Device);
			if(!status)
				usbDevPwr = 1;
		}
	}
	else
	{
		if(((func == USB_HOST_FUNC) || (func == USB_ALL_FUNC)) && usbHostPwr)
		{
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Host);
			if(!status)
				usbHostPwr = 0;
		}
		
		if(((func == USB_DEVICE_FUNC) || (func == USB_ALL_FUNC)) && usbDevPwr)
		{
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Device);
			if(!status)
					usbDevPwr = 0;
		}
		
		if(!usbHostPwr && !usbDevPwr)
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_Common);
	}
#endif
#endif

	return status;
}
EXPORT_SYMBOL(kerSysSetUsbPower);

extern const struct obs_kernel_param __setup_start[], __setup_end[];
extern const struct kernel_param __start___param[], __stop___param[];

void kerSysSetBootParm(char *name, char *value)
{
    const struct obs_kernel_param *okp = __setup_start;
    const struct kernel_param *kp = __start___param;

    do {
        if (!strcmp(name, okp->str)) {
            if (okp->setup_func) {
                okp->setup_func(value);
                return;
            }
        }
        okp++;
    } while (okp < __setup_end);

    do {
        if (!strcmp(name, kp->name)) {
            if (kp->ops->set) {
                kp->ops->set(value, kp);
                return;
            }
        }
        kp++;
    } while (kp < __stop___param);
}
EXPORT_SYMBOL(kerSysSetBootParm);



//====================================================================================
// Default Button Callbacks: 
/* These are the commonly used callbacks.   It is possible to register other
   callbacks using the RegisterPushButtonPress/Hold/Release functions outside
   of boardparms */


/***************************************************************************/
// BP_BNT_CBACK_NONE
void btnHook_DoNothing(unsigned long timeInMs, unsigned long param) {    
    // do nothing...
}


/***************************************************************************/
// BP_BTN_ACTION_RESTORE_DEFAULTS
void btnHook_RestoreToDefault(unsigned long timeInMs, unsigned long param) {    
    if( !restore_in_progress )
    {
        char buf[256] = {};
        restore_in_progress  = 1;

        printk(" *** Restore to Factory Default Setting ***\n\n");
        kerSysPersistentSet( buf, sizeof(buf), 0 );
#if defined(CONFIG_BCM_PLC_BOOT)
        kerSysFsFileSet("/data/plc/plc_pconfig_state", buf, 1);
#endif
        kernel_restart(NULL);        
    }    
}

/***************************************************************************/
// BP_BTN_ACTION_PRINT
void btnHook_Print(unsigned long timeInMs, unsigned long param) {    
    printk("%s\n", (char *)param);
}


/***************************************************************************/
// BP_BTN_ACTION_SES
void btnHook_Ses(unsigned long timeInMs, unsigned long param) {
#if defined(WIRELESS)
    unsigned long flags;

    printk(" *** Doing SES *** (%pf)\n\n", sesBtn_defaultAction);

    /* This one is a bit trickier, as the wireless does not accept sockets
       etc.  Thus, the wireless is going to poll to see whether the button
       was pressed.  What we do, is we record the time in jiffies.  When the
       wireless polls, we check if we were within 200ms of the last press */
    gLastSesBtnEvTime = jiffies;
    gSesBtnEvOutstanding = 1;

    /* Synchronization note: This code is protected with
     * sesBtn_newapi_spinlock to avoid a race condition with the
     * reading/writing of sesBtn_active in sesBtn_read
     */
    spin_lock_irqsave(&sesBtn_newapi_spinlock, flags);
    switch (param) {
    case SES_BTN_PARAM_AP:
        atomic_set(&sesBtn_active, SES_BTN_AP);
        break;
    case SES_BTN_PARAM_STA:
        atomic_set(&sesBtn_active, SES_BTN_STA);
        break;
    default:
        atomic_set(&sesBtn_active, SES_BTN_AP);
        break;
    }
    spin_unlock_irqrestore(&sesBtn_newapi_spinlock, flags);

    sesBtn_defaultAction(timeInMs, param);
#endif
}

/***************************************************************************/
// BP_BTN_ACTION_WLAN_DOWN
void btnHook_WlanDown(unsigned long timeInMs, unsigned long param) {
#if defined(WIRELESS)
    struct net_device *wlan_dev;

    printk("Bringing down wireless interface wl0\n");
    wlan_dev = dev_get_by_name(&init_net, "wl0");
    if (wlan_dev)
    {
        netif_carrier_off(wlan_dev);
        rtnl_lock();
        dev_close(wlan_dev);
        rtnl_unlock();
        dev_put(wlan_dev);
    }
#endif
}

/***************************************************************************/
// BP_BTN_ACTION_PLC_UKE

static pushButtonNotifyHook_t gPlcUkeCallback = NULL;
unsigned long gPlcUkeCallbackParam = 0;
static pushButtonNotifyHook_t gPlcRandomizeCallback = NULL;
unsigned long gPlcRandomizeCallbackParam = 0;
static DEFINE_SPINLOCK(btn_spinlock);

void kerSysRegisterPlcUkeCallback( void (* callback)(unsigned long timeInMs, unsigned long param), unsigned long param ) {
    unsigned long flags;

    printk("Registering Plc Uke callback: (%pf)\n", callback);
    
    spin_lock_irqsave(&btn_spinlock, flags);
    gPlcUkeCallback = callback;
    gPlcUkeCallbackParam = param;
    spin_unlock_irqrestore(&btn_spinlock, flags);
}

void kerSysDeregisterPlcUkeCallback( void (* callback)(unsigned long timeInMs, unsigned long param) ) {
    unsigned long flags;
    printk("Deregistering Plc Uke callback: (%pf)\n", callback);
    spin_lock_irqsave(&btn_spinlock, flags);
    if (gPlcUkeCallback == callback)
        gPlcUkeCallback = NULL;
    spin_unlock_irqrestore(&btn_spinlock, flags);
}

void kerSysRegisterPlcRandomizeCallback(void (* callback)(unsigned long timeInMs,
                                                          unsigned long param),
                                        unsigned long param) {
    unsigned long flags;

    printk("Registering PLC network key randomize callback: (%pf)\n", callback);

    spin_lock_irqsave(&btn_spinlock, flags);
    gPlcRandomizeCallback = callback;
    gPlcRandomizeCallbackParam = param;
    spin_unlock_irqrestore(&btn_spinlock, flags);
}

void kerSysDeregisterPlcRandomizeCallback(void (* callback)(unsigned long timeInMs,
                                                            unsigned long param)) {
    unsigned long flags;
    printk("Deregistering PLC network key randomize callback: (%pf)\n", callback);
    spin_lock_irqsave(&btn_spinlock, flags);
    if (gPlcRandomizeCallback == callback)
        gPlcRandomizeCallback = NULL;
    spin_unlock_irqrestore(&btn_spinlock, flags);
}

void btnHook_PlcUke(unsigned long timeInMs, unsigned long param) {
    unsigned long flags;
    pushButtonNotifyHook_t hook;
    unsigned long hookParam;
    
    printk(" *** Doing PLC UKE (%pf) ***\n\n", gPlcUkeCallback);
 
    spin_lock_irqsave(&btn_spinlock, flags);
    hook=gPlcUkeCallback;
    hookParam=gPlcUkeCallbackParam;
    spin_unlock_irqrestore(&btn_spinlock, flags);
    if (hook)
        hook(timeInMs, hookParam);
}

/***************************************************************************/
// BP_BTN_ACTION_RANDOMIZE_PLC
void btnHook_RandomizePlc(unsigned long timeInMs, unsigned long param) {
    unsigned long flags;
    pushButtonNotifyHook_t hook;
    unsigned long hookParam;

    printk(" *** Randomizing PLC ***\n");

    spin_lock_irqsave(&btn_spinlock, flags);
    hook=gPlcRandomizeCallback;
    hookParam=gPlcRandomizeCallbackParam;
    spin_unlock_irqrestore(&btn_spinlock, flags);
    if (hook)
        hook(timeInMs, hookParam);
}


/***************************************************************************/
// BP_BTN_ACTION_RESET
void btnHook_Reset(unsigned long timeInMs, unsigned long param) {
    printk(" *** Restarting System ***\n\n");
    kernel_restart(NULL);
}




pushButtonNotifyHook_t btnHooks[] = {
    [BP_BTN_ACTION_NONE >> BP_BTN_ACTION_SHIFT]             = (pushButtonNotifyHook_t)btnHook_DoNothing,
    [BP_BTN_ACTION_SES  >> BP_BTN_ACTION_SHIFT]             = (pushButtonNotifyHook_t)btnHook_Ses,  
    [BP_BTN_ACTION_RESTORE_DEFAULTS  >> BP_BTN_ACTION_SHIFT]= (pushButtonNotifyHook_t)btnHook_RestoreToDefault,  
    [BP_BTN_ACTION_RANDOMIZE_PLC>> BP_BTN_ACTION_SHIFT]     = (pushButtonNotifyHook_t)btnHook_RandomizePlc,
    [BP_BTN_ACTION_RESET>> BP_BTN_ACTION_SHIFT]             = (pushButtonNotifyHook_t)btnHook_Reset,
    [BP_BTN_ACTION_PRINT>> BP_BTN_ACTION_SHIFT]             = (pushButtonNotifyHook_t)btnHook_Print,
    [BP_BTN_ACTION_PLC_UKE>> BP_BTN_ACTION_SHIFT]           = (pushButtonNotifyHook_t)btnHook_PlcUke,
    [BP_BTN_ACTION_WLAN_DOWN>> BP_BTN_ACTION_SHIFT]         = (pushButtonNotifyHook_t)btnHook_WlanDown,
};


//====================================================================================
// Main Button Support: 


#define BTN_EV_PRESSED       0x1
#define BTN_EV_HOLD          0x2
#define BTN_EV_RELEASED      0x4
#define BTN_POLLFREQ         100         /* in ms */

// Main button structure:
typedef struct _BtnInfo {
    PB_BUTTON_ID         btnId;
    int                  extIrqIdx;    //this is NOT 0 based... (platform specific index)
    int                  gpio;         //zero based index
    bool                 activeHigh;
    int                  active;       //set to 1 if button is down, 0 otherwise
    bool                 isConfigured;
    
    uint32_t             lastPressJiffies;
    uint32_t             lastHoldJiffies;
    uint32_t             lastReleaseJiffies;
    
    struct timer_list    timer;        //used for polling
    
    spinlock_t            lock;
    unsigned long         events;       //must be protected by lock
    wait_queue_head_t     waitq;
    struct task_struct *  thread;
    
    //interrupt related functions
    irqreturn_t  (* pressIsr)(int irq, void *btnInfo);
    irqreturn_t  (* releaseIsr)(int irq, void *btnInfo);
    void         (* poll)(unsigned long btnInfo);

    //functional related fuctions
    bool         (* isDown)(struct _BtnInfo *btnInfo);
    void         (* enableIrqs)(struct _BtnInfo *btnInfo);
    

} BtnInfo;

static BtnInfo btnInfo[PB_BUTTON_MAX] = {};

static void btnDoPress(BtnInfo *btn, unsigned long currentJiffies);
static void btnDoRelease(BtnInfo *btn, unsigned long currentJiffies);
static void btnDoHold(BtnInfo *btn, unsigned long currentJiffies);

/***************************************************************************
 * Function Name: btnThread
 * Description  : This is the thread function that takes care of a button.
                  It is repsonsible for invoking any registered call backs,
                  and doing polling. 
                  Assume it will never exit...
 * Parameters   : arg: pointer to the button structure
 ***************************************************************************/
int btnThread(void * arg) {
    BtnInfo *btn = (BtnInfo*)arg;
    unsigned long flags;
    struct sched_param sp = { .sched_priority = 20 };

    // set to be realtime thread with somewhat high priority:
    sched_setscheduler(current, SCHED_FIFO, &sp);

    while(1) {     

        // at this point the button is not pressed -- wait for press:
        wait_event_interruptible(btn->waitq, (btn->events & BTN_EV_PRESSED) != 0);
        
        spin_lock_irqsave(&btn->lock, flags);        
        btn->events &= ~ BTN_EV_PRESSED;
        spin_unlock_irqrestore(&btn->lock, flags);
        btnDoPress(btn, btn->lastPressJiffies);

        // at this point the button is down -- wait for release or until next hold event:
        while(1) {
            // TBD: instead of waking up every 100ms, we can actually poll the pushButton
            // library to figure out when the next wakeup time should be.
            // Note: all times should be read by the ISR, as the thread can be delayed
            // in heavy traffic, giving inacurate results if read from here
            wait_event_interruptible_timeout(btn->waitq, btn->events != 0, msecs_to_jiffies(BTN_POLLFREQ));
            if (btn->events & BTN_EV_HOLD) {
                btn->events &= ~BTN_EV_HOLD;
                btnDoHold(btn, btn->lastHoldJiffies);
            }
            if (btn->events & BTN_EV_RELEASED) {
                spin_lock_irqsave(&btn->lock, flags);        
                btn->events &= ~ (BTN_EV_RELEASED | BTN_EV_HOLD);
                spin_unlock_irqrestore(&btn->lock, flags);
                btnDoRelease(btn, btn->lastReleaseJiffies);
                break;
            }
        }
    }
}


/***************************************************************************
 * Function Name: btnPressIsr
 * Description  : This is the default btnPress interrupt handler.  It
                  assumes the button drives a gpio and is mapped to an
                  external interrupt.
                  This invokes the doPushButtonPress, and starts the
                  polling timer.
 * Parameters   : irq: the irq number of the button
                  info: a pointer to a BtnInfo structure
 * Returns      : IRQ_HANDLED.
 ***************************************************************************/
static irqreturn_t btnPressIsr(int irq, void *info) {
    BtnInfo *btn = (BtnInfo*)info;
    unsigned long currentJiffies = jiffies;
    int wasTimerActive;
    unsigned long flags;
    
    if (IsExtIntrShared(extIntrInfo[btn->extIrqIdx]) && !btn->isDown(btn)) {
        return IRQ_HANDLED;
    }
    
    spin_lock_irqsave(&btn->lock, flags);   
    btn->active=1;
    btn->lastPressJiffies=currentJiffies;
    if ( btn->releaseIsr == NULL && btn->poll != NULL){
        wasTimerActive = mod_timer(&btn->timer, (currentJiffies + msecs_to_jiffies(BTN_POLLFREQ)));
    }
    btn->events |= BTN_EV_PRESSED;
    wake_up(&btn->waitq);
    spin_unlock_irqrestore(&btn->lock, flags);
    return IRQ_HANDLED;
}

/***************************************************************************
 * Function Name: btnReleaseIsr
 * Description  : This is the default hw btn release interrupt handler.  It
                  assumes the button drives a gpio and is mapped to an
                  external interrupt.  It is only called if a button is edge
                  detected on both (up and down) edges.
                  This stops the polling timer, and invokes the doRelease
                  callback. 
 * Parameters   : irq: the irq number of the button
                  info: a pointer to a BtnInfo structure
 * Returns      : IRQ_HANDLED.
 ***************************************************************************/
static irqreturn_t btnReleaseIsr(int irq, void *info) {

    BtnInfo *btn = (BtnInfo*)info;
    unsigned long flags;
    
    if (IsExtIntrShared(extIntrInfo[btn->extIrqIdx]) && !btn->isDown(btn))
        return IRQ_HANDLED;
    
    spin_lock_irqsave(&btn->lock, flags);
    btn->active=0;
    btn->lastReleaseJiffies = jiffies;
    del_timer(&btn->timer);
    btn->events |= BTN_EV_RELEASED;
    wake_up(&btn->waitq);
    spin_unlock_irqrestore(&btn->lock, flags);
    return IRQ_HANDLED;
}


/***************************************************************************
 * Function Name: btnIsGpioBtnDown
 * Description  : This a the check to see if a gpio-based button is down
                  based on the gpio level
 * Parameters   : arg: a pointer to a BtnInfo structure
 * Returns      : 1 if the button is down
 ***************************************************************************/
static bool btnIsGpioBtnDown(BtnInfo *btn) {
    // check hardware to see if button is actually down:
    int value;
#if !defined(CONFIG_BCM960333)
    value = kerSysGetGpioValue(btn->gpio);  //TBD -- not supported for 60333 yet
#else
    value = GPIO->GPIOData & (1 << btn->gpio);
#endif
    if( !!value == !!btn->activeHigh ) {        
        return TRUE;    
    } else {
        return FALSE;
    }
}

/***************************************************************************
 * Function Name: btnPoll
 * Description  : This is the polling function.  It is started when a 
                  button press is detected, and stopped when a button 
                  release is detected.
 * Parameters   : arg: a pointer to a BtnInfo structure
 ***************************************************************************/
static void btnPoll(unsigned long arg) {
    BtnInfo *btn = (BtnInfo *)arg;
    unsigned long currentJiffies = jiffies;
    int wasTimerActive;
    unsigned long flags;

    spin_lock_irqsave(&btn->lock, flags);
    if (btn->active) {
        if ( btn->isDown(btn) ) {
            btn->lastHoldJiffies = currentJiffies;
            wasTimerActive=mod_timer(&btn->timer, currentJiffies + msecs_to_jiffies(BTN_POLLFREQ)); 
            btn->events |= BTN_EV_HOLD;
            wake_up(&btn->waitq);
        }
        else if (btn->releaseIsr == NULL) {
            btn->lastReleaseJiffies = currentJiffies;
            btn->active = 0;
            del_timer(&btn->timer);
            btn->events |= BTN_EV_RELEASED;
            wake_up(&btn->waitq);
        } 
        else {
            // hit race condition.  releaseIsr is pending (and it will 
            // stop the timer, etc.   Do nothing here)
        }
    } 
    else {
        // we should not get here
    }
    
    spin_unlock_irqrestore(&btn->lock, flags);
}


/***************************************************************************
 * Function Name: btnDoPress
 * Description  : This is called when a press has been detected.
 * Parameters   : arg: a pointer to a BtnInfo structure
 ***************************************************************************/
static void btnDoPress (BtnInfo *btn, unsigned long currentJiffies) {
    // this is called from the kernel thread context.
    doPushButtonPress(btn->btnId, currentJiffies);
    if (btn->releaseIsr) {
            btn->enableIrqs(btn);
    }
    return;
}

/***************************************************************************
 * Function Name: btnDoRelease
 * Description  : This is called when a release has been detected
 * Parameters   : arg: a pointer to a BtnInfo structure
 ***************************************************************************/
static void btnDoRelease (BtnInfo *btn, unsigned long currentJiffies) {
    // this is called from the kernel thread context.
    doPushButtonRelease(btn->btnId, currentJiffies);    
    if (btn->pressIsr) {
            btn->enableIrqs(btn);
    }
    return;
}


/***************************************************************************
 * Function Name: btnDoHold
 * Description  : This is called when a button hold is detected
 * Parameters   : arg: a pointer to a BtnInfo structure
 ***************************************************************************/
static void btnDoHold(BtnInfo *btn, unsigned long currentJiffies) {
    // this is called from the kernel thread context.
    doPushButtonHold(btn->btnId, currentJiffies);
}


/***************************************************************************
 * Function Name: btnEnableIrq
 * Description  : enable a bttons Irqs
 ***************************************************************************/
static void btnEnableIrq(BtnInfo *btn) {
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    BcmHalExternalIrqUnmask(btn->extIrqIdx);
#else
    BcmHalInterruptEnable(btn->extIrqIdx);
#endif
}

/***************************************************************************
 * Function Name: registerBtns
 * Description  : This parses the board parms and sets up all buttons
                  accordingly.
 ***************************************************************************/

/* Parse board parms and read button Id's. */
/* For now, assume gpio button with ext interrupt.  This can be modified
   going forward however, by changing the callbacks, etc
*/
static int registerBtns(void)
{
    void *          iter=NULL;                   // iterator
    int             ret;
    unsigned short  bpBtnIdx, bpGpio, bpExtIrq;  // values read from boardparms
    unsigned short  btnIdx;                      // actual button index
    unsigned short  gpioNum;                     // actual gpio number
    unsigned short  extIrqIdx;                      // actual ext irq
    unsigned short  isActiveHigh;                // iff true signal is high when button is pressed
    unsigned short  hasDownIsr;                  // This is true if isr fires on both up and down 
    BtnInfo *       btn;
    unsigned long   flags;
    unsigned short  bpNumHooks = MAX_BTN_HOOKS_PER_BTN;
    unsigned short  bpHooks[MAX_BTN_HOOKS_PER_BTN];
    unsigned long   bpHookParms[MAX_BTN_HOOKS_PER_BTN];
    
    while(1) {
        bpNumHooks = MAX_BTN_HOOKS_PER_BTN;
        ret=BpGetButtonInfo(&iter, &bpBtnIdx, &bpGpio, &bpExtIrq, &bpNumHooks, bpHooks, bpHookParms);
        if (ret != BP_SUCCESS) {
            break;
        }
        if (bpBtnIdx >= PB_BUTTON_MAX) {
            printk("registerBtns: Button index %d out of range (max %d)\n", bpBtnIdx, PB_BUTTON_MAX);
            return -1;
        }
        
        if (bpGpio == BP_NOT_DEFINED) {
            printk("ERROR: registerBtns: GPIO not set for button %d (not handled yet)\n", bpBtnIdx);
            return -1;
        }
        
        if (bpExtIrq == BP_EXT_INTR_NONE) {
            //eventually this will be done via polling.
            printk("ERROR: registerBtns: ExtIrq not set for button %d (not handled yet)\n", bpBtnIdx);
            return -1;
        }

        btnIdx = bpBtnIdx;        
        hasDownIsr = 0;    // not supported as of yet
        gpioNum = bpGpio & BP_GPIO_NUM_MASK;
        isActiveHigh = ((bpGpio & BP_ACTIVE_MASK) == BP_ACTIVE_HIGH);
        extIrqIdx = (bpExtIrq & ~BP_EXT_INTR_FLAGS_MASK) - BP_EXT_INTR_0;
        if (isActiveHigh != IsExtIntrTypeActHigh(bpExtIrq)) {
            printk("registerBtns: Error -- mismatch on activehigh/low for button %d\n", bpBtnIdx);
            return -1;
        }
        if (IsExtIntrShared(extIntrInfo[bpExtIrq])) {
            printk("Error -- shared button (%d) interrupts not handled yet...\n", bpBtnIdx);
            return -1;
        }
        if (IsExtIntrConflict(extIntrInfo[bpExtIrq])) {
            printk("Error -- Btn conflicting interrupts not handled yet...\n");
            return -1;
        }


        btn = &btnInfo[btnIdx];
        
        printk("Registering button %d (%p) (bpGpio: %08x, bpExtIrq:%08x)\n", 
            bpBtnIdx, btn, bpGpio, bpExtIrq);

        printk("    %s extIrqIdx:%d gpioNum:%d\n", 
                isActiveHigh?"ACTIVE HIGH":"ACTIVE LOW", extIrqIdx, gpioNum);

        if (btn->isConfigured != 0) {
            printk("Warning -- button %d defined twice in boardparms \n", btn->btnId);
            // overriding old definition...
            memset(btn, 0, sizeof(*btn));
        }
        btn->isConfigured = 1;
        
        spin_lock_init(&btn->lock);
        
        btn->btnId = PB_BUTTON_0 + btnIdx;
        btn->gpio = gpioNum;
        btn->extIrqIdx = extIrqIdx;
        btn->active = FALSE;
        btn->activeHigh = isActiveHigh;
        btn->events = 0;
        init_waitqueue_head(&btn->waitq);
        
        btn->thread = kthread_run(btnThread, (void *)btn, "btnhandler%d", btnIdx);
        if (!btn->thread) {
            printk("ERROR could not start kthread\n");   
            continue;  
        }
        
        spin_lock_irqsave(&btn->lock, flags);

        // set up data:

        /* The following is the default callbacks (assuming gpio to extIrq).  For any
           other type of button, simply replace the callbacks */
        btn->pressIsr = btnPressIsr; 
        btn->releaseIsr = hasDownIsr ? btnReleaseIsr : NULL;
        btn->poll = btnPoll;
        btn->isDown = btnIsGpioBtnDown;
        btn->enableIrqs = btnEnableIrq;


        // set up timer:
        if (btn->releaseIsr == NULL) {
            // we're going to have to poll the button to see when it's released:
            init_timer(&btn->timer);
            btn->timer.function = btn->poll;
            btn->timer.expires  = jiffies + msecs_to_jiffies(BTN_POLLFREQ);
            btn->timer.data     = (unsigned int)btn;
        }

        // set up external interrupts / gpios:
        
        btn->extIrqIdx = map_external_irq (bpExtIrq);


#if defined(CONFIG_BCM960333)
        if (IsExtIntrShared(extIntrInfo[btn->extIrqIdx]))
            printk("Error -- Cannot share ext irqs in 60333 (due to mux... %d)\n", btn->extIrqIdx);
        mapBcm960333GpioToIntr(btn->gpio, bpExtIrq);
#elif defined(CONFIG_BCM963381) || defined(CONFIG_BCM96848)
        kerSysInitPinmuxInterface(BP_PINMUX_FNTYPE_IRQ | btn->extIrqIdx );
#endif


        if (btn->pressIsr || btn->releaseIsr)
            BcmHalMapInterrupt((FN_HANDLER)btn->pressIsr, (unsigned int)btn, btn->extIrqIdx);
            
#if !defined(CONFIG_ARM)
        if (btn->pressIsr || btn->releaseIsr)
            BcmHalInterruptEnable(btn->extIrqIdx);
#endif

        // Register button hooks:
        {
            int idx;
            for (idx = 0; idx < bpNumHooks; idx++) {
                unsigned short bpHook = bpHooks[idx];                
                unsigned long bpHookParm = bpHookParms[idx];
                int timeInMs = (bpHook & BP_BTN_TRIG_TIME_MASK) * BP_BTN_TRIG_TIME_UNIT_IN_MS;
                int bpType = bpHook & BP_BTN_TRIG_TYPE_MASK;
                pushButtonNotifyHook_t hook = btnHooks[(bpHook & BP_BTN_ACTION_MASK) >> BP_BTN_ACTION_SHIFT ];

                printk("  Button %d: Registering %s hook %pf after %d ms \n", 
                             btn->btnId, 
                             bpType==BP_BTN_TRIG_PRESS?"press":bpType==BP_BTN_TRIG_HOLD?"hold":"release",
                             hook, timeInMs);
                
                switch (bpType) {
                    case BP_BTN_TRIG_PRESS:
                        registerPushButtonPressNotifyHook(btn->btnId, hook, bpHookParm);
                        break;
                    case BP_BTN_TRIG_HOLD:
                        registerPushButtonHoldNotifyHook(btn->btnId, hook, timeInMs, bpHookParm);
                        break;
                    case BP_BTN_TRIG_RELEASE:
                        registerPushButtonReleaseNotifyHook(btn->btnId, hook, timeInMs, bpHookParm);
                        break;
                }
                
            }
        }
        
        spin_unlock_irqrestore(&btn->lock, flags);

        
    }
    return 0;
}

/***************************************************************************
* MACRO to call driver initialization and cleanup functions.
***************************************************************************/
module_init( brcm_board_init );
module_exit( brcm_board_cleanup );

EXPORT_SYMBOL(dumpaddr);
EXPORT_SYMBOL(kerSysGetChipId);
EXPORT_SYMBOL(kerSysGetChipName);
EXPORT_SYMBOL(kerSysMacAddressNotifyBind);
EXPORT_SYMBOL(kerSysGetMacAddressType);
EXPORT_SYMBOL(kerSysGetMacAddress);
EXPORT_SYMBOL(kerSysReleaseMacAddress);
EXPORT_SYMBOL(kerSysGetGponSerialNumber);
EXPORT_SYMBOL(kerSysGetGponPassword);
EXPORT_SYMBOL(kerSysGetSdramSize);
#if !defined(CONFIG_BCM960333)
EXPORT_SYMBOL(kerSysGetDslPhyEnable);
EXPORT_SYMBOL(kerSysGetExtIntInfo);
#endif
EXPORT_SYMBOL(kerSysSetOpticalPowerValues);
EXPORT_SYMBOL(kerSysGetOpticalPowerValues);
EXPORT_SYMBOL(kerSysLedCtrl);
EXPORT_SYMBOL(kerSysRegisterDyingGaspHandler);
EXPORT_SYMBOL(kerSysDeregisterDyingGaspHandler);
EXPORT_SYMBOL(kerSysIsDyingGaspTriggered);
EXPORT_SYMBOL(kerSysSendtoMonitorTask);
EXPORT_SYMBOL(kerSysGetWlanSromParams);
EXPORT_SYMBOL(kerSysGetWlanFeature);
EXPORT_SYMBOL(kerSysGetAfeId);
EXPORT_SYMBOL(kerSysGetUbusFreq);
EXPORT_SYMBOL(kerSysInitPinmuxInterface);
#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM96838)
EXPORT_SYMBOL(kerSysBcmSpiSlaveRead);
EXPORT_SYMBOL(kerSysBcmSpiSlaveReadReg32);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWrite);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWriteReg32);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWriteBuf);
#if defined(CONFIG_BCM_6802_MoCA)
EXPORT_SYMBOL(kerSysBcmSpiSlaveReadBuf);
EXPORT_SYMBOL(kerSysBcmSpiSlaveModify);
#endif
EXPORT_SYMBOL(kerSysMocaHostIntrReset);
EXPORT_SYMBOL(kerSysRegisterMocaHostIntrCallback);
EXPORT_SYMBOL(kerSysMocaHostIntrEnable);
EXPORT_SYMBOL(kerSysMocaHostIntrDisable);
#endif


#if defined(CONFIG_BCM_WATCHDOG_TIMER)
EXPORT_SYMBOL(kerSysRegisterWatchdogCB);
EXPORT_SYMBOL(kerSysDeregisterWatchdogCB);
EXPORT_SYMBOL(bcm_suspend_watchdog);
EXPORT_SYMBOL(bcm_resume_watchdog);
#endif

EXPORT_SYMBOL(BpGetSimInterfaces);
EXPORT_SYMBOL(BpGetBoardId);
EXPORT_SYMBOL(BpGetBoardIds);
EXPORT_SYMBOL(BpGetGPIOverlays);
EXPORT_SYMBOL(BpGetFpgaResetGpio);
EXPORT_SYMBOL(BpGetEthernetMacInfo);
EXPORT_SYMBOL(BpGetEthernetMacInfoArrayPtr);
EXPORT_SYMBOL(BpGetGphyBaseAddress);
EXPORT_SYMBOL(BpGetDeviceOptions);
EXPORT_SYMBOL(BpGetPortConnectedToExtSwitch);
EXPORT_SYMBOL(BpGetRj11InnerOuterPairGpios);
EXPORT_SYMBOL(BpGetRtsCtsUartGpios);
EXPORT_SYMBOL(BpGetAdslLedGpio);
EXPORT_SYMBOL(BpGetAdslFailLedGpio);
EXPORT_SYMBOL(BpGetWanDataLedGpio);
EXPORT_SYMBOL(BpGetWanErrorLedGpio);
EXPORT_SYMBOL(BpGetVoipLedGpio);
EXPORT_SYMBOL(BpGetPotsLedGpio);
EXPORT_SYMBOL(BpGetVoip2FailLedGpio);
EXPORT_SYMBOL(BpGetVoip2LedGpio);
EXPORT_SYMBOL(BpGetVoip1FailLedGpio);
EXPORT_SYMBOL(BpGetVoip1LedGpio);
EXPORT_SYMBOL(BpGetDectLedGpio);
EXPORT_SYMBOL(BpGetMoCALedGpio);
EXPORT_SYMBOL(BpGetMoCAFailLedGpio);
#if defined(GPL_CODE)
EXPORT_SYMBOL(BpGetUsbLedGpio);
EXPORT_SYMBOL(BpGetBootloaderPowerOnLedGpio);
EXPORT_SYMBOL(BpGetBootloaderStopLedGpio);
EXPORT_SYMBOL(BpGetWirelessFailSesLedGpio);
#if defined(GPL_CODE_63168_CHIP)
EXPORT_SYMBOL(BpGetEnetWanLedGpio);
#endif
#endif
EXPORT_SYMBOL(BpGetWirelessSesExtIntr);
EXPORT_SYMBOL(BpGetWirelessSesLedGpio);
EXPORT_SYMBOL(BpGetWirelessFlags);
EXPORT_SYMBOL(BpGetWirelessPowerDownGpio);
EXPORT_SYMBOL(BpUpdateWirelessSromMap);
EXPORT_SYMBOL(BpGetSecAdslLedGpio);
EXPORT_SYMBOL(BpGetSecAdslFailLedGpio);
EXPORT_SYMBOL(BpGetDslPhyAfeIds);
EXPORT_SYMBOL(BpGetExtAFEResetGpio);
EXPORT_SYMBOL(BpGetExtAFELDPwrGpio);
EXPORT_SYMBOL(BpGetExtAFELDModeGpio);
EXPORT_SYMBOL(BpGetIntAFELDPwrGpio);
EXPORT_SYMBOL(BpGetIntAFELDModeGpio);
EXPORT_SYMBOL(BpGetAFELDRelayGpio);
EXPORT_SYMBOL(BpGetExtAFELDDataGpio);
EXPORT_SYMBOL(BpGetExtAFELDClkGpio);
EXPORT_SYMBOL(BpGetAFEVR5P3PwrEnGpio);
EXPORT_SYMBOL(BpGetUart2SdoutGpio);
EXPORT_SYMBOL(BpGetUart2SdinGpio);
EXPORT_SYMBOL(BpGet6829PortInfo);
EXPORT_SYMBOL(BpGetEthSpdLedGpio);
EXPORT_SYMBOL(BpGetLaserDisGpio);
EXPORT_SYMBOL(BpGetLaserTxPwrEnGpio);
EXPORT_SYMBOL(BpGetVregSel1P2);
EXPORT_SYMBOL(BpGetVregAvsMin);
EXPORT_SYMBOL(BpGetGponOpticsType);
EXPORT_SYMBOL(BpGetDefaultOpticalParams);
EXPORT_SYMBOL(BpGetI2cGpios);
EXPORT_SYMBOL(BpGetMiiOverGpioFlag);
EXPORT_SYMBOL(BpGetSwitchPortMap);
EXPORT_SYMBOL(BpGetMocaInfo);
EXPORT_SYMBOL(BpGetPhyResetGpio);
EXPORT_SYMBOL(BpGetPhyAddr);
EXPORT_SYMBOL(BpGetBatteryEnable);

#if defined (CONFIG_BCM_ENDPOINT_MODULE)
EXPORT_SYMBOL(BpGetVoiceBoardId);
EXPORT_SYMBOL(BpGetVoiceBoardIds);
EXPORT_SYMBOL(BpGetVoiceParms);
#endif

#if defined(CONFIG_EPON_SDK)
EXPORT_SYMBOL(BpGetNumFePorts);
EXPORT_SYMBOL(BpGetNumGePorts);
EXPORT_SYMBOL(BpGetNumVoipPorts);
#endif
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
EXPORT_SYMBOL(BpGetPonTxEnGpio);
EXPORT_SYMBOL(BpGetPonRxEnGpio);
EXPORT_SYMBOL(BpGetPonResetGpio);
#endif
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963148)
EXPORT_SYMBOL(BpGetExtAFELDPwrDslCtl);
EXPORT_SYMBOL(BpGetExtAFELDModeDslCtl);
EXPORT_SYMBOL(BpGetIntAFELDPwrDslCtl);
EXPORT_SYMBOL(BpGetIntAFELDModeDslCtl);
EXPORT_SYMBOL(BpGetIntAFELDDataDslCtl);
EXPORT_SYMBOL(BpGetIntAFELDClkDslCtl);
EXPORT_SYMBOL(BpGetExtAFELDDataDslCtl);
EXPORT_SYMBOL(BpGetExtAFELDClkDslCtl);
EXPORT_SYMBOL(BpGetSgmiiGpios);
#endif

#ifdef GPL_CODE
EXPORT_SYMBOL(kerSysGetDslDatapump);
#endif
#if defined(GPL_CODE)
EXPORT_SYMBOL(AEI_getCurrentUpWanType);
#if defined(GPL_CODE_CHIP_D0_CHECK)
EXPORT_SYMBOL(CPURevId);
EXPORT_SYMBOL(CPUSerialNumber);
#endif
#endif
#ifdef GPL_CODE_TWO_IN_ONE_FIRMWARE
EXPORT_SYMBOL(kerSysGetBoardID);
#endif

EXPORT_SYMBOL(BpGetOpticalWan);
EXPORT_SYMBOL(BpGetRogueOnuEn);
EXPORT_SYMBOL(BpGetGpioLedSim);
EXPORT_SYMBOL(BpGetGpioLedSimITMS);

#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM96848)
EXPORT_SYMBOL(BpGetTsyncPonUnstableGpio);
EXPORT_SYMBOL(BpGetTsync1ppsPin);
EXPORT_SYMBOL(BpGetPmdAlarmExtIntr);
EXPORT_SYMBOL(BpGetPmdAlarmExtIntrGpio);
EXPORT_SYMBOL(BpGetPmdSDExtIntr);
EXPORT_SYMBOL(BpGetPmdSDExtIntrGpio);
EXPORT_SYMBOL(BpGetPmdMACEwakeEn);
EXPORT_SYMBOL(BpGetGpioPmdReset);

EXPORT_SYMBOL(BpGetTrplxrTxFailExtIntr);
EXPORT_SYMBOL(BpGetTrplxrTxFailExtIntrGpio);
EXPORT_SYMBOL(BpGetTrplxrSdExtIntr);
EXPORT_SYMBOL(BpGetTrplxrSdExtIntrGpio);
EXPORT_SYMBOL(BpGetTxLaserOnOutN);
EXPORT_SYMBOL(BpGet1ppsStableGpio);
EXPORT_SYMBOL(BpGetLteResetGpio);
EXPORT_SYMBOL(BpGetStrapTxEnGpio);
EXPORT_SYMBOL(BpGetWifiOnOffExtIntr);
EXPORT_SYMBOL(BpGetWifiOnOffExtIntrGpio);
EXPORT_SYMBOL(BpGetLteExtIntr);
EXPORT_SYMBOL(BpGetLteExtIntrGpio);
EXPORT_SYMBOL(BpGetAePolarity);
EXPORT_SYMBOL(BpGetWanSignalDetectedGpio);

#endif


EXPORT_SYMBOL(BpGetUsbPwrOn0);
EXPORT_SYMBOL(BpGetUsbPwrOn1);
EXPORT_SYMBOL(BpGetUsbPwrFlt0);
EXPORT_SYMBOL(BpGetUsbPwrFlt1);
#if defined(GPL_CODE) && defined(GPL_CODE_63138_CHIP)
EXPORT_SYMBOL(BpGetWifiLedGpio);
#endif


EXPORT_SYMBOL(BpGetAttachedInfo);

MODULE_LICENSE("GPL");
