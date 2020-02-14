/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2006:DUAL/GPL:standard
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
 *
 ************************************************************************/


/* OS dependent messaging functions go here */

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <fcntl.h>
#include <errno.h>
#include "../oal.h"
#include "cms_util.h"

#ifdef GPL_CODE_MYNETWORK
/*
 * Set socket option
 * here only set SO_SNDBUF, SO_RCVBUF, SO_SNDTIMEO, SO_RCVTIMEO
 *
 * When set SO_SNDBUF/SO_RCVBUF, value is buffer size of BYTE
 * When set SO_SNDTIMEO/SO_RCVTIMEO, value is milliseconds
 */
int set_socket_opt(int fd, int opt, int value)
{
    int ret = 0;
    int bufsize, len;
    struct timeval  timeout;

    switch(opt)
    {
        case SO_SNDBUF:
        case SO_RCVBUF:
            bufsize = value;
            len = sizeof(bufsize);
            ret = setsockopt(fd, SOL_SOCKET, opt, (char *)&bufsize, len);
        break;
        case SO_SNDTIMEO:
        case SO_RCVTIMEO:
        {
            /* the value is timeout in ms */
            timeout.tv_sec= value/1000;
            timeout.tv_usec= (value % 1000)*1000;
            len = sizeof(timeout);
            ret = setsockopt(fd, SOL_SOCKET, opt, (char *)&timeout, len);
        }
        break;
        default:
        break;
    }
    if(ret < 0)
        printf("[%s] setsockopt %d, errno=%s\n", __func__, opt, strerror(errno));

    return ret;
}
#endif
#if defined(GPL_CODE_CMS_MSG_NOBLOCK)
int cmsMsg_blockLog(char *str)
{
    int ret = -1;
    struct stat st;
    int fd = -1;

    if(str == NULL || str[0] == '\0')
        return ret;

    fd = open(CMS_MSG_BLOCK_LOG, O_CREAT|O_RDWR|O_APPEND);
    if(fd > 0){
        fstat(fd, &st);
        if(st.st_size < MAX_CMS_MSG_LOG){
            ret = write(fd, str, strlen(str));
        }else{
            cmsLog_error(CMS_MSG_BLOCK_LOG" has reached the maximum!");
            ret = 0;
        }
        close(fd);
    }

    return ret;
}
/*
 * This functions is used to set cms msg socket noblock. In order to avoid cms msg send block.
 * The noblock means short timeout for blocking mode here. It is not non-blocking mode actually.
 * Init long timeout, when send block on socket and the long timeout, change to short timeout.
 * When cms msg system recovery, the socket can send succeeded again, change to long timeout.
 */
int set_msg_socket_snd_timeout(int fd, UBOOL8 *blkState, UBOOL8 noblock, const CmsMsgHeader *hdr)
{
    int ret = 0;

    if(*blkState != noblock)
    {
        struct timespec ts;
        char buffer[BUFLEN_256] = {0};
        clock_gettime(CLOCK_MONOTONIC, &ts);

        if(noblock)
        {
            ret = set_socket_opt(fd, SO_SNDTIMEO, AEI_CMS_MSG_NOBLOCK_SNDTIMEO);
            if(!ret){
                snprintf(buffer,sizeof(buffer)-1,"Pid(%d) eid(%d) send msg(%x) to eid(%d) failed on socket(%d), set snd timeout short wait. uptime(%d).\n",getpid(), hdr->src, hdr->type, hdr->dst, fd, (int)ts.tv_sec);
                cmsMsg_blockLog(buffer);
                cmsLog_error(buffer);
                *blkState = noblock;
            }else{
                cmsLog_error("%d set failed on %d! errno: %d",getpid(),fd,errno);
            }
        }
        else
        {
            unsigned long usedsize= 0u;
            int bufsize = 0;
            int len = sizeof(bufsize);
            ret = ioctl(fd, SIOCOUTQ, &usedsize);
            ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, (socklen_t *)&len);

            /* When process msg very slowly but not held. In order to avoid switching mode Frequently.
             * When the free buffer is more than half, switching to long timeout mode */
            if(usedsize <= (bufsize/2))
            {
                ret = set_socket_opt(fd, SO_SNDTIMEO, AEI_CMS_MSG_DEFAULT_SNDTIMEO);
                if(!ret){
                    snprintf(buffer,sizeof(buffer)-1,"Pid(%d) eid(%d) send msg(%x) to eid(%d) succeeded again on socket(%d) and usedsize drop to a half bufsize, set snd timeout long wait. uptime(%d).\n",getpid(), hdr->src, hdr->type, hdr->dst, fd, (int)ts.tv_sec);
                    cmsMsg_blockLog(buffer);
                    cmsLog_error(buffer);
                    *blkState = noblock;
                }else{
                    cmsLog_error("%d set failed on %d! errno: %d",getpid(),fd,errno);
                }
            }else{
                cmsLog_error("usedsize (%d) > a half bufsize (%d), pid: %d",usedsize,bufsize/2,getpid());
            }
        }
    }

    return ret;
}
#endif


CmsRet oalMsg_initWithFlags(CmsEntityId eid, UINT32 flags, void **msgHandle)
{
   CmsMsgHandle *handle;
   struct sockaddr_un serverAddr;
   SINT32 rc;

   
   if ((handle = (CmsMsgHandle *) cmsMem_alloc(sizeof(CmsMsgHandle), ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate storage for msg handle");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* store caller's eid */
   handle->eid = eid;

#ifdef DESKTOP_LINUX
   /*
    * Applications may be run without smd on desktop linux, so if we
    * don't see a socket for smd, don't bother connecting to it.
    */
   {
      struct stat statbuf;

      if ((rc = stat(SMD_MESSAGE_ADDR, &statbuf)) < 0)
      {
         handle->commFd = CMS_INVALID_FD;
         handle->standalone = TRUE;
         *msgHandle = (void *) handle;
         cmsLog_notice("no smd server socket detected, running in standalone mode.");
         return CMSRET_SUCCESS;
      }
   }
#endif /* DESKTOP_LINUX */


      /*
       * Create a unix domain socket.
       */
      handle->commFd = socket(AF_LOCAL, SOCK_STREAM, 0);
      if (handle->commFd < 0)
      {
         cmsLog_error("Could not create socket");
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }


      /*
       * Set close-on-exec, even though all apps should close their
       * fd's before fork and exec.
       */
      if ((rc = fcntl(handle->commFd, F_SETFD, FD_CLOEXEC)) != 0)
      {
         cmsLog_error("set close-on-exec failed, rc=%d errno=%d", rc, errno);
         close(handle->commFd);
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }


      /*
       * Connect to smd.
       */
      memset(&serverAddr, 0, sizeof(serverAddr));
      serverAddr.sun_family = AF_LOCAL;
      strncpy(serverAddr.sun_path, SMD_MESSAGE_ADDR, sizeof(serverAddr.sun_path));

      rc = connect(handle->commFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
      if (rc != 0)
      {
         cmsLog_error("connect to %s failed, rc=%d errno=%d", SMD_MESSAGE_ADDR, rc, errno);
         close(handle->commFd);
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("commFd=%d connected to smd", handle->commFd);
      }

#if defined(GPL_CODE_CMS_MSG_NOBLOCK)
      set_socket_opt(handle->commFd, SO_SNDTIMEO, AEI_CMS_MSG_DEFAULT_SNDTIMEO);
      handle->sndNoBlock = FALSE;
#endif

      /* send a launched message to smd */
      {
         CmsRet ret;
         CmsMsgHeader launchMsg = EMPTY_MSG_HEADER;

         launchMsg.type = CMS_MSG_APP_LAUNCHED;
         launchMsg.src = (flags & EIF_MULTIPLE_INSTANCES) ? MAKE_SPECIFIC_EID(getpid(), eid) : eid;
         launchMsg.dst = EID_SMD;
         launchMsg.flags_event = 1;
         launchMsg.wordData = getpid();

         if ((ret = oalMsg_send(handle->commFd, &launchMsg)) != CMSRET_SUCCESS)
         {
            close(handle->commFd);
            cmsMem_free(handle);
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("sent LAUNCHED message to smd");
         }
      }

   /* successful, set handle pointer */
   *msgHandle = (void *) handle;

   return CMSRET_SUCCESS;
}


void oalMsg_cleanup(void **msgHandle)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) *msgHandle;

   if (handle->commFd != CMS_INVALID_FD)
   {
      close(handle->commFd);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR((*msgHandle));

   return;
}


CmsRet oalMsg_getEventHandle(const CmsMsgHandle *msgHandle, void *eventHandle)
{
   SINT32 *fdPtr = (SINT32 *) eventHandle;

   *fdPtr = msgHandle->commFd;

   return CMSRET_SUCCESS;
}


CmsRet oalMsg_send(SINT32 fd, const CmsMsgHeader *buf)
{
   UINT32 totalLen;
   SINT32 rc;
   CmsRet ret=CMSRET_SUCCESS;

   totalLen = sizeof(CmsMsgHeader) + buf->dataLength;

   rc = write(fd, buf, totalLen);
   if (rc < 0)
   {
      if (errno == EPIPE)
      {
         /*
          * This could happen when smd tries to write to an app that
          * has exited.  Don't print out a scary error message.
          * Just return an error code and let upper layer app handle it.
          */
         cmsLog_debug("got EPIPE, dest app is dead");
         return CMSRET_DISCONNECTED;
      }
      else
      {
         cmsLog_error("write failed, errno=%d", errno);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (rc != (SINT32) totalLen)
   {
      cmsLog_error("unexpected rc %d, expected %u", rc, totalLen);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


static CmsRet waitForDataAvailable(SINT32 fd, UINT32 timeout)
{
   struct timeval tv;
   fd_set readFds;
   SINT32 rc;

   FD_ZERO(&readFds);
   FD_SET(fd, &readFds);

   tv.tv_sec = timeout / MSECS_IN_SEC;
   tv.tv_usec = (timeout % MSECS_IN_SEC) * USECS_IN_MSEC;

   rc = select(fd+1, &readFds, NULL, NULL, &tv);
   if ((rc == 1) && (FD_ISSET(fd, &readFds)))
   {
      return CMSRET_SUCCESS;
   }
   else
   {
      return CMSRET_TIMED_OUT;
   }
}


CmsRet oalMsg_receive(SINT32 fd, CmsMsgHeader **buf, UINT32 *timeout)
{
   CmsMsgHeader *msg;
   SINT32 rc;
   CmsRet ret;

   if (buf == NULL)
   {
      cmsLog_error("buf is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }
   else
   {
      *buf = NULL;
   }

   if (timeout)
   {
      if ((ret = waitForDataAvailable(fd, *timeout)) != CMSRET_SUCCESS)
      {
         return ret;
      }
   }

   /*
    * Read just the header in the first read.
    * Do not try to read more because we might get part of 
    * another message in the TCP socket.
    */
   msg = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   if (msg == NULL)
   {
#ifdef GPL_CODE_MYNETWORK
/*In case smd is stuck, csmLog_xxx won't work*/
      printf("ATTENTION!!! alloc of msg header failed in %s\n", __func__);
#endif
      cmsLog_error("alloc of msg header failed");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   rc = read(fd, msg, sizeof(CmsMsgHeader));
   if ((rc == 0) ||
       ((rc == -1) && (errno == 131)))  /* new 2.6.21 kernel seems to give us this before rc==0 */
   {
      /* broken connection */
      cmsMem_free(msg);
      return CMSRET_DISCONNECTED;
   }
   else if (rc < 0 || rc != sizeof(CmsMsgHeader))
   {
      cmsLog_error("bad read, rc=%d errno=%d", rc, errno);
      cmsMem_free(msg);
      return CMSRET_INTERNAL_ERROR;
   }

   if (msg->dataLength > 0)
   {
      UINT32 totalReadSoFar=0;
      SINT32 totalRemaining=msg->dataLength;
      char *inBuf;
      CmsMsgHeader *newMsg;

      /* there is additional data in the message */
      newMsg = (CmsMsgHeader *) cmsMem_realloc(msg, sizeof(CmsMsgHeader) + msg->dataLength);
      if (newMsg == NULL)
      {
#ifdef GPL_CODE_MYNETWORK
/*In case smd is stuck, csmLog_xxx won't work*/
         printf("ATTENTION!!! realloc of msg body failed in %s\n", __func__);
#endif
#ifdef GPL_CODE_COVERITY_FIX
         cmsLog_error("realloc bytes failed");
#else
         cmsLog_error("realloc to %d bytes failed", sizeof(CmsMsgHeader) + msg->dataLength);
#endif
         cmsMem_free(msg);
         return CMSRET_RESOURCE_EXCEEDED;
      }

      /* orig msg was freed by cmsMem_realloc, so now msg can point to newMsg */
      msg = newMsg;
      newMsg = NULL;

      inBuf = (char *) (msg + 1);
      while (totalReadSoFar < msg->dataLength)
      {
         cmsLog_debug("reading segment: soFar=%u total=%d", totalReadSoFar, totalRemaining);
         if (timeout)
         {
            if ((ret = waitForDataAvailable(fd, *timeout)) != CMSRET_SUCCESS)
            {
               cmsMem_free(msg);
               return ret;
            }
         }

         rc = read(fd, inBuf, totalRemaining);
         if (rc <= 0)
         {
            cmsLog_error("bad data read, rc=%d errno=%d readSoFar=%d remaining=%d", rc, errno, totalReadSoFar, totalRemaining);
            cmsMem_free(msg);
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            inBuf += rc;
            totalReadSoFar += rc;
#ifdef GPL_CODE_COVERITY_FIX
	    if(totalRemaining >= rc)
	    {
		    totalRemaining -= rc;
	    }
	    else
	    {
		    totalRemaining = 0;
	    }
#else
            totalRemaining -= rc;
#endif
         }
      }
   }

   *buf = msg;

   return CMSRET_SUCCESS;
}

