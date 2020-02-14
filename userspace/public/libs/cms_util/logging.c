/***********************************************************************
 *
<:copyright-BRCM:2006:DUAL/GPL:standard

   Copyright (c) 2006 Broadcom Corporation
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
 *
************************************************************************/


#include <fcntl.h>      /* open */ 
#include "cms.h"
#include "cms_log.h"
#include "cms_eid.h"
#include "cms_mem.h"
#include "oal.h"

/** local definitions **/

/* default settings */

/** external functions **/

/** external data **/

/** internal functions **/

/** public data **/

/** private data **/
static CmsEntityId gEid;     /**< try not to use this.  inefficient b/c requires eInfo lookup. */
static char *gAppName=NULL;  /**< name of app; set during init */
static CmsLogLevel             logLevel; /**< Message logging level.
                                          * This is set to one of the message
                                          * severity levels: LOG_ERR, LOG_NOTICE
                                          * or LOG_DEBUG. Messages with severity
                                          * level lower than or equal to logLevel
                                          * will be logged. Otherwise, they will
                                          * be ignored. This variable is local
                                          * to each process and is changeable
                                          * from CLI.
                                          */ 
static CmsLogDestination logDestination; /**< Message logging destination.
                                          * This is set to one of the
                                          * message logging destinations:
                                          * STDERR or SYSLOGD. This
                                          * variable is local to each
                                          * process and is changeable from
                                          * CLI.
                                          */
static UINT32 logHeaderMask; /**< Bitmask of which pieces of info we want
                              *   in the log line header.
                              */ 

void log_log(CmsLogLevel level, const char *func, UINT32 lineNum, const char *pFmt, ... )
{
   va_list		ap;
   char buf[MAX_LOG_LINE_LENGTH] = {0};
   int len=0, maxLen;
   char *logLevelStr=NULL;
   int logTelnetFd = -1;

   maxLen = sizeof(buf);

   if (level <= logLevel)
   {
      va_start(ap, pFmt);

      if (logHeaderMask & CMSLOG_HDRMASK_APPNAME)
      {
         if (NULL != gAppName)
         {
            len = snprintf(buf, maxLen, "%s:", gAppName);
         }
         else
         {
            len = snprintf(buf, maxLen, "%d:", cmsEid_getPid());
         }
      }


      if ((logHeaderMask & CMSLOG_HDRMASK_LEVEL) && (len < maxLen))
      {
         /*
          * Only log the severity level when going to stderr
          * because syslog already logs the severity level for us.
          */
         if (logDestination == LOG_DEST_STDERR)
         {
            switch(level)
            {
            case LOG_LEVEL_ERR:
               logLevelStr = "error";
               break;
            case LOG_LEVEL_NOTICE:
               logLevelStr = "notice";
               break;
#ifdef GPL_CODE_CMS_LOG_DEBUG
            case LOG_LEVEL_AEIDEBUG:
               logLevelStr = "aeidebug";
               break;
#endif
            case LOG_LEVEL_DEBUG:
               logLevelStr = "debug";
               break;
            default:
               logLevelStr = "invalid";
               break;
            }
            len += snprintf(&(buf[len]), maxLen - len, "%s:", logLevelStr);
         }
      }


      /*
       * Log timestamp for both stderr and syslog because syslog's
       * timestamp is when the syslogd gets the log, not when it was
       * generated.
       */
      if ((logHeaderMask & CMSLOG_HDRMASK_TIMESTAMP) && (len < maxLen))
      {
         CmsTimestamp ts;

         cmsTms_get(&ts);
         len += snprintf(&(buf[len]), maxLen - len, "%u.%03u:",
                         ts.sec%1000, ts.nsec/NSECS_IN_MSEC);
      }


      if ((logHeaderMask & CMSLOG_HDRMASK_LOCATION) && (len < maxLen))
      {
         len += snprintf(&(buf[len]), maxLen - len, "%s:%u:", func, lineNum);
      }


      if (len < maxLen)
      {
         maxLen -= len;
         vsnprintf(&buf[len], maxLen, pFmt, ap);
      }

      if (logDestination == LOG_DEST_STDERR)
      {
         fprintf(stderr, "%s\n", buf);
         fflush(stderr);
      }
      else if (logDestination == LOG_DEST_TELNET )
      {
   #ifdef DESKTOP_LINUX
         /* Fedora Desktop Linux */
         logTelnetFd = open("/dev/pts/1", O_RDWR);
   #else
#ifdef GPL_CODE
/*
 * On purpose of debug via telnet,
 * We need to redirect some out put to telnet terminal with
 * > logdest set xxxx Telnet
 * by default it will redirect to /dev/ttyp0, which is the first terminal
 * but sometimes we engineer is not the first one to access by telnet
 * In this case, we need to change the tty output to exactly the one we use.
 *
 * Do:
 * 1.Try to find which tty we are using:
 *   # echo "Where are we" > /dev/ttyp0
 *   # echo "Where are we" > /dev/ttyp1
 *   ....
 *   until we find "where are we" output on our terminal
 * 2.Make soft link /dev/ttypx to /data/ttypx
 *   ln -sf /dev/ttypx /data/ttypx
 * 3.redirect output to current telnet terminal
 *   > logdest set xxx Telnet
 * 
 */
        logTelnetFd = open("/data/ttypx", O_RDWR);
        if(logTelnetFd == -1)
        {
            logTelnetFd = open("/dev/ttyp0", O_RDWR);
        }
#else
         /* CPE use ptyp0 as the first pesudo terminal */
         logTelnetFd = open("/dev/ttyp0", O_RDWR);
#endif
   #endif
         if(logTelnetFd != -1)
         {
            if (0 > write(logTelnetFd, buf, strlen(buf)))
               printf("write to telnet fd failed\n");
            if (0 > write(logTelnetFd, "\n", strlen("\n")))
               printf("write to telnet fd failed(2)\n");
            close(logTelnetFd);
         }
      }
      else
      {
         oalLog_syslog(level, buf);
      }

      va_end(ap);
   }

}  /* End of log_log() */

#if defined(GPL_CODE)
/*
 * Function: AEI_check_log_file_size()
 *         
 * Description: check file log size, if file size > 32k, we will rotate the device log, else do nothing
 *                    
 * Input Parameters: None
 *                           
 * Return values:  0, success
 *                -1, fault                          
 */
int AEI_check_log_file_size(void)
{
    char    myCmd[1024];
    long    iMaxFileSize = 32 * 1024;
    long    iFileSize;
    char    *szLogFile = DEVICE_HISTORY_LOG_FILE;
    char    *szTmpFile = DEVICE_HISTORY_LOG_FILE".bak";
    FILE    *fp;
    FILE    *fpTmp;
    long    iStart;
    char    *szBuf;
    int     iBufSize = 32*1024;
    int     iReadSize;
    int     iWriteSize;

    fp = fopen(szLogFile, "r+b");
    if (!fp)
    {
        printf("open file %s failed\n", szLogFile);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    iFileSize = ftell(fp);
    if (iFileSize < iMaxFileSize)
    {
        fclose(fp);
        return -1;
    }

    iStart = iFileSize - iMaxFileSize/2;
    fseek(fp, iStart, SEEK_SET);

    fpTmp = fopen(szTmpFile, "w+b");
    if (!fpTmp)
    {
        fclose(fp);
        return -1;
    }

    szBuf = (char*)malloc(iBufSize);
    if (!szBuf)
    {
        fclose(fp);
        fclose(fpTmp);
        return -1;
    }

    //locate file first.
    while(!feof(fp))
    {
        iReadSize = fread(szBuf, 1, 1, fp);
        if (iReadSize <= 0)
        {
            break;
        }

        if (szBuf[0] == '\n')
        {
            break;
        }
    }

    while(!feof(fp))
    {
        iReadSize = fread(szBuf, 1, iBufSize, fp);
        if (iReadSize <= 0)
        {
            break;
        }

        iWriteSize = fwrite(szBuf, 1, iReadSize, fpTmp);
        if (iWriteSize != iReadSize)
        {
            fclose(fp);
            fclose(fpTmp);
            free(szBuf);
            return -1;
        }
    }

    fclose(fp);
    fclose(fpTmp);
    free(szBuf);

    sprintf(myCmd, "cp -f %s %s", szTmpFile, szLogFile);
    system(myCmd);

    sprintf(myCmd, "rm -f %s", szTmpFile);
    system(myCmd);

    return 0;
}

/*
 * Function: AEI_log_local_info()
 *        
 * Description: write some necessary device log to file.
 *             
 * Input Parameters: szName    which process write device log.
 *                   szInfo    the device log info.
 *                        
 * Return values: None                 
 */
void AEI_log_local_info(char *szName, char *szInfo)
{
    char    myLog[512];
    char    myCmd[1024];
    time_t  ulTime = time(NULL);
    struct tm   stLocalTime;
    char        *szLogFile = DEVICE_HISTORY_LOG_FILE;

    memcpy(&stLocalTime, localtime(&ulTime), sizeof(stLocalTime));
    sprintf(myLog, "%04d-%02d-%02d %02d:%02d:%02d "
                   "-%10s",
                   (1900+stLocalTime.tm_year), (1+stLocalTime.tm_mon), stLocalTime.tm_mday,
                   stLocalTime.tm_hour, stLocalTime.tm_min, stLocalTime.tm_sec,
                   szName);

    snprintf(myCmd, sizeof(myCmd)-1, "echo \"%s %s\">>%s", myLog, szInfo, szLogFile);
    myCmd[sizeof(myCmd)-1] = '\0';

    system(myCmd);

    //check device log file size.
    AEI_check_log_file_size();
}
#endif
    
void cmsLog_initWithName(CmsEntityId eid, const char *appName)
{
   logLevel       = DEFAULT_LOG_LEVEL;
   logDestination = DEFAULT_LOG_DESTINATION;
   logHeaderMask  = DEFAULT_LOG_HEADER_MASK;

   gEid = eid;

   /*
    * highly unlikely this strdup will fail, but even if it does, the
    * code can handle a NULL gAppName.
    */
   gAppName = cmsMem_strdup(appName);

   oalLog_init();

   return;
}


void cmsLog_init(CmsEntityId eid)
{
   const CmsEntityInfo *eInfo;

   if (NULL != (eInfo = cmsEid_getEntityInfo(eid)))
   {
      cmsLog_initWithName(eid, eInfo->name);
   }
   else
   {
      cmsLog_initWithName(eid, NULL);
   }

   return;

}  /* End of cmsLog_init() */


void cmsLog_cleanup(void)
{
   oalLog_cleanup();
   CMSMEM_FREE_BUF_AND_NULL_PTR(gAppName);
   return;

}  /* End of cmsLog_cleanup() */
  

void cmsLog_setLevel(CmsLogLevel level)
{
   logLevel = level;
   return;
}


CmsLogLevel cmsLog_getLevel(void)
{
   return logLevel;
}


void cmsLog_setDestination(CmsLogDestination dest)
{
   logDestination = dest;
   return;
}


CmsLogDestination cmsLog_getDestination(void)
{
   return logDestination;
}


void cmsLog_setHeaderMask(UINT32 headerMask)
{
   logHeaderMask = headerMask;
   return;
}


UINT32 cmsLog_getHeaderMask(void)
{
   return logHeaderMask;
} 


int cmsLog_readPartial(int ptr, char* buffer)
{
   return (oal_readLogPartial(ptr, buffer));
}
