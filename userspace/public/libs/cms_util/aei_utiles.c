#if defined(GPL_CODE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <net/if.h>
#include "cms_util.h"
#include "oal.h"
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>
#include "aei_utiles.h"
#if defined(GPL_CODE_CONFIG_JFFS)
#include "bcmTag.h" /* in shared/opensource/include/bcm963xx, for FILE_TAG */
#include "board.h" /* in bcmdrivers/opensource/include/bcm963xx, for BCM_IMAGE_CFE */
#include "cms_boardioctl.h"
#endif
#define READ_BUF_SIZE 128


int AEI_get_value_by_file(char *file, int size, char *value)
{
    FILE *fp = NULL;

    if((fp = fopen(file, "r")) != NULL)
    {
        fgets(value, size - 1 , fp);
        fclose(fp);
        return 1;
    }
    else
        return 0;
}


UINT16 AEI_get_interface_mtu(char *ifname)
{
   struct ifreq ifr;
   int sockfd, err;

#ifdef GPL_CODE_COVERITY_FIX
   if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
#else
   if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0 )
#endif
   {
      return -1;
   }

#ifdef GPL_CODE_COVERITY_FIX
   cmsUtl_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
#else
   strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
#endif
   err = ioctl(sockfd, SIOCGIFMTU, (void*)&ifr);
   close(sockfd);

   if(err < 0)
       return 1500; //default MTU
   else
       return ifr.ifr_mtu;
}
void AEI_createFile(char *filename, char *content)
{
    FILE *fp=NULL;
    if((fp=fopen(filename,"w"))==NULL)
   {//file exist
        printf("write %s failed\n",filename);
        return;
    }
    if(content != NULL)
    fprintf(fp, "%s", content);
    fclose(fp);
}


int AEI_removeFile(char *filename)
{
   int ret = 0;
    if (remove(filename) != 0) {
            cmsLog_error("Could not remove %s file=%d", filename);
            ret = -1;
    }
    return ret;
}

/* 0- file exist*/
int AEI_isFileExist(char *filename)
{
   int fileFlag=access(filename,F_OK);
   return fileFlag;
}
int AEI_get_mac_addr(char *ifname, char *mac)
{
	int fd, rtn;
	struct ifreq ifr;

	if( !ifname || !mac ) {
		return -1;
	}
	fd = socket(AF_INET, SOCK_DGRAM, 0 );
	if ( fd < 0 ) {
		perror("socket");
		return -1;
	}
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

	if ((rtn = ioctl(fd, SIOCGIFHWADDR, &ifr)) == 0)
	{
         sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                 (UINT8)(ifr.ifr_hwaddr.sa_data[0]),(UINT8)(ifr.ifr_hwaddr.sa_data[1]),
                 (UINT8)(ifr.ifr_hwaddr.sa_data[2]),(UINT8)(ifr.ifr_hwaddr.sa_data[3]),
                 (UINT8)(ifr.ifr_hwaddr.sa_data[4]),(UINT8)(ifr.ifr_hwaddr.sa_data[5]));
	}
	close(fd);
	return rtn;
}

int AEI_convert_space(char *src,char *dst)
{
    int i=0;
    int len=0;
    int j=0;
    if(src==NULL || dst==NULL)
        return -1;
    len=strlen(src);
    for(i=0;i<len;i++)
    {
        if(src[i]==' ')
        {
            dst[j]='%';
            j++;
            sprintf(&dst[j],"20");
            j+=2;
        }
        else
        {
            dst[j]=src[i];
            j++;
        }
    }
    return 0;
}


int AEI_convert_spec_chars(char *src,char *dst)
{
    int len=0;
    int i=0;
    int j=0;
    if(src==NULL || dst == NULL)
        return -1;

    len=strlen(src);
    for(i=0;i<len;i++)
    {
        switch(src[i])
        {
            case ':':
            case ';':
            case '|':
            case '"':
            case ' ':
            case '+':
            case '\'':
            case '\\':
                dst[j]='%';
                j++;
                sprintf(&dst[j],"%02x",src[i]);
                j+=2;
                break;
            default:
                dst[j]=src[i];
                j++;
        }
    }
    return 0;
}

/* Encode the output value base on microsoft standard*/
char* AEI_SpeciCharEncode(char *s, int len)
{
    int c;
    int n = 0;
    char t[BUFLEN_4096]={0};
    char *p;

    p=s;
    memset(t,0,sizeof(t));

    if (s == NULL) {
        cmsLog_error("The transfer object is null");
        return s;
    }
    while ((c = *p++)&& n < BUFLEN_4096) {
        /*
        Yuki: Special characters |,/()+ \are used by join characters, also need Encode them.
        */

#ifdef GPL_CODE_COVERITY_FIX
	if(n == BUFLEN_4096-1)
	{
		    cmsLog_error("The Array size overflow Exception");
		    return s;
	}
#endif
#if defined(GPL_CODE_GUI_SECURITY)
        if (!strchr("<>\"'%;)(&+|,/\\:", c))
#else
        if (!strchr("<>\"'%;)(&+|,/\\", c))
#endif
        {
            t[n++] = c;
        } else if (n < BUFLEN_4096-5) {
            t[n++] = '&';
            t[n++] = '#';
            /*
            Yuki:| encode is &#124;
            */
            if(strchr("|",c)){
                t[n++] = (c/100) + '0';
                t[n++] = ((c%100)/10) + '0';
                t[n++] = ((c%100)%10) + '0';
            }else{
                t[n++] = (c/10) + '0';
                t[n++] = (c%10) + '0';
            }
            t[n++] = ';';
        } else {
            cmsLog_error("The Array size overflow Exception");
            return s;;
        }
    }
    t[n] = '\0';
    if (n <= len && n < BUFLEN_4096)
    {
        memset(s,0,len);
        strncpy(s, t,  n);
    }
    else
        cmsLog_error("The Array size overflow Exception");
    return s;
}

char *AEI_getGUIStrValue(char *v_in, char *v_out, int v_out_len)
{
    if (v_in && strlen(v_in)>0)
    {
        cmsUtl_strncpy(v_out, v_in, v_out_len);
#ifdef GPL_CODE_GUI_SECURITY
        AEI_SpeciCharEncode(v_out, v_out_len);
#endif
        return v_out;
    }
    else{
        return v_in;
    }
}

char *AEI_getGUIStrValue2(char *v_in, int v_in_len)
{
    if (v_in && strlen(v_in)>0)
    {
#ifdef GPL_CODE_GUI_SECURITY
        AEI_SpeciCharEncode(v_in, v_in_len);
#endif
        return v_in;
    }
    else{
        return v_in;
    }
}


static void remove_delimitor( char *s)
{
    char *p1, *p2;

    p1 = p2 = s;
    while ( *p1 != '\0' || *(p1+1) != '\0') {
        if (*p1 != '\0') {
           *p2 = *p1;
           p2++;
         }
         p1++;
    }
    *p2='\0';

}


pid_t* find_pid_by_name( char* pidName)
{
        DIR *dir;
        struct dirent *next;
        pid_t* pidList=NULL;
        int i=0;

        /*FILE *status */
        FILE *cmdline;
        char filename[READ_BUF_SIZE];
        char buffer[READ_BUF_SIZE];
        /* char name[READ_BUF_SIZE]; */

        dir = opendir("/proc");
        if (!dir) {
                printf("cfm:Cannot open /proc");
                return NULL;
        }

        while ((next = readdir(dir)) != NULL) {
                /* re-initialize buffers */
                memset(filename, 0, sizeof(filename));
                memset(buffer, 0, sizeof(buffer));

                /* Must skip ".." since that is outside /proc */
                if (strcmp(next->d_name, "..") == 0)
                        continue;

                /* If it isn't a number, we don't want it */
                if (!isdigit(*next->d_name))
                        continue;

                /* sprintf(filename, "/proc/%s/status", next->d_name); */
                /* read /porc/<pid>/cmdline instead to get full cmd line */
                sprintf(filename, "/proc/%s/cmdline", next->d_name);
                if (! (cmdline = fopen(filename, "r")) ) {
                        continue;
                }
                if (fgets(buffer, READ_BUF_SIZE-1, cmdline) == NULL) {
                        fclose(cmdline);
                        continue;
                }
                fclose(cmdline);

                /* Buffer should contain a string like "Name:   binary_name" */
                /*sscanf(buffer, "%*s %s", name);*/
                /* buffer contains full commandline params separted by '\0' */
                remove_delimitor(buffer);
                if (strstr(buffer, pidName) != NULL) {
                        pidList=realloc( pidList, sizeof(pid_t) * (i+2));
                        if (!pidList) {
                                printf("cfm: Out of memeory!\n");
				closedir(dir);
                                return NULL;
                        }
                        pidList[i++]=strtol(next->d_name, NULL, 0);
                }
        }
        closedir(dir);

        if (pidList)
                pidList[i]=0;
        else if ( strcmp(pidName, "init")==0) {
                /* If we found nothing and they were trying to kill "init",
                 * guess PID 1 and call it good...  Perhaps we should simply
                 * exit if /proc isn't mounted, but this will do for now. */
                pidList=realloc( pidList, sizeof(pid_t));
                if (!pidList) {
                        printf("cfm: Out of memeory!\n");
                        return NULL;
                }
                pidList[0]=1;
        } else {
                pidList=realloc( pidList, sizeof(pid_t));
                if (!pidList) {
                        printf("cfm: Out of memeory!\n");
                        return NULL;
                }
                pidList[0]=-1;
        }
        return pidList;
}


int AEI_GetPid(char * command)
{
    char cmdline[128], *p1, *p2;
    pid_t *pid = NULL;
    int ret = 0;

    p1 = command;
    p2 = cmdline;
    while ( *p1 != '\0') {
        if (*p1 != ' ') {
           *p2 = *p1;
           p2++;
         }
         p1++;
    }
    *p2='\0';

    pid = find_pid_by_name(cmdline);
    if ( pid != NULL ) {
       ret = (int)(*pid);
       free(pid);
    }

    return ret;
}


#if defined(GPL_CODE_CONFIG_JFFS)
#include "bcm_flashutil.h"
CmsRet AEI_writeDualPartition(char *imagePtr, UINT32 imageLen, void *msgHandle, int partition)
{
   CmsImageFormat format;
#ifdef GPL_CODE_COVERITY_FIX
   CmsRet ret = CMSRET_SUCCESS;
#else
   CmsRet ret;
#endif

   if ((format = cmsImg_validateImage(imagePtr, imageLen, msgHandle)) == CMS_IMAGE_FORMAT_INVALID)
   {
      ret = CMSRET_INVALID_IMAGE;
   }
#if defined(GPL_CODE)
   else if(format == CMS_IMAGE_FORMAT_CORRUPTED)
   {
      ret = CMSRET_INVALID_IMAGE;
   }
#endif
   else if(format == CMS_IMAGE_FORMAT_FLASH)
   {
       if ( (ret = writeDualImageToNand(imagePtr, imageLen - TOKEN_LEN, partition)) == CMSRET_SUCCESS )
       { 
            devCtl_boardIoctl(BOARD_IOCTL_MIPS_SOFT_RESET, 0, NULL, 0, 0, 0);
       }
   }
   return ret;
}
#endif
#if defined(GPL_CODE)
int AEI_save_syslog()
{
    return 1;
}
#endif
#endif
#ifdef GPL_CODE_LOCKOUT_AGAINST_BRUTE_FORCE_ATTACKS
UINT32 getUptime()
{

    CmsTimestamp ts;
    cmsTms_get(&ts);
    return ts.sec;
}

CmsRet getCliLockInfo(UINT32* max_retry, UINT32* curr_retry, UINT32* timeout)
{
    FILE* fd = NULL;
    char buf[64] = {0};
    fd = fopen("/data/cli_lock_info","r");
    if (fd)
    {
        fgets(buf,sizeof(buf)-1,fd);
        *curr_retry = atoi(buf);
        fgets(buf,sizeof(buf)-1,fd);
        *max_retry = atoi(buf);
        fgets(buf,sizeof(buf)-1,fd);
        *timeout = atoi(buf);
        fclose(fd);
    }
    else
    {
        cmsLog_error("create /data/cli_lock_info file");
        *max_retry = 4;
        *curr_retry = 0;
        *timeout = 900;
        saveCliLockInfo(*max_retry, *curr_retry, *timeout);
    }
    //
    cmsLog_debug("max_retry=%d,curr_retry=%d,timeout=%d",* max_retry, * curr_retry, * timeout);

    return CMSRET_SUCCESS;
}

CmsRet saveCliLockInfo(UINT32 max_retry, UINT32 curr_retry, UINT32 timeout)
{
    FILE* fd = NULL;
    fd = fopen("/data/cli_lock_info","w");
    if (fd)
    {
        fprintf(fd,"%d\n",curr_retry);
        fprintf(fd,"%d\n",max_retry);
        fprintf(fd,"%d\n",timeout);
        fclose(fd);
    }
    else
    {
        cmsLog_error("create /data/cli_lock_info failed");
        return CMSRET_INTERNAL_ERROR;
    }

    return CMSRET_SUCCESS;
}


void saveCliLockUptime(char *appname)
{
    FILE* fd = NULL;

    fd = fopen("/var/cli_lock_uptime","w");
    if (fd)
    {
        fprintf(fd,"%d\n",getUptime() );
        fprintf(fd,"appName=%s\n", appname);
        /*fprintf(fd,"ipAddr=%s\n", pSLD->ipAddr);
        fprintf(fd,"port=%d\n", pSLD->port);
        fprintf(fd,"user=%s\n", pSLD->user);*/
        fclose(fd);
    }
}

UBOOL8 isCliLocked(UINT32 timeout)
{
    FILE* fd = NULL;
    char buf[64] = {0};
    UINT32 lockTime = 0;
    UINT32 upTimeNow = getUptime();
    int    delta = 0;
    fd = fopen("/var/cli_lock_uptime","r");
    if (fd)
    {
        fgets(buf,sizeof(buf)-1,fd);
        fclose(fd);
    }
    lockTime = atoi(buf);
    delta = upTimeNow - lockTime;
    //
    //fprintf(stderr,"\nUp Time : expired(%d),now(%d), last(%d)\n",expiredTime,upTimeNow,expiredTime-upTimeNow);
    if (lockTime > 0 && delta < timeout)
    {
        return TRUE;
    }

    return FALSE;
}

#endif

/*
 *It is used to get idx from wlmngr msg, and reduce 1 for bit mappping in SMD and used in wlmngr.
 *For example Modify:5, it will return 5.
 **/
int AEI_get_wifi_idx_from_msg(char *msg, int index)
{
    int len = -1;
    int idx = 0;
    int i = 0;

    if ((index < 1) || (msg == NULL))
        return -1;

    len = strlen(msg);
    if (len <= index)
        return -1;

    for (i = index; i < len; i++)
    {
        idx = (idx * 10) + (msg[i] - '0');
    }

    return idx;
}

/*
 * AEI_getCmdOutPut - get the input command result
 * @command:     [in]       the inputted command
 * @len          [in]       the result value length
 * @value        [out]      the pointer to output result
 * Returns: the pointer to output result
 */
char* AEI_getCmdOutPut(char* command, char* value, int len)
{
   char *cmd = command;
   FILE *fp = NULL;

   fp = popen(cmd,"r");
   if(NULL==fp)
   {
       cmsLog_error("popen error");
       return NULL;
   }
   if(NULL != fgets(value, len,fp))
   {
       strtok(value, "\n");
   }
   if(fp)
   {
       pclose(fp);
   }
   return value;
}


/*
 * AEI_isHexStringValid - check the given hex string is valid
 * @str          [in]       the given hex string
 * Returns: 0 - invalid / 1 - valid
 */
int AEI_isHexStringValid(char *str)
{
    int i=0;
    int lenStr=0;

    if(NULL==str)
        return 0;

    lenStr=strlen(str);
    if(0==lenStr)
        return 0;

    if(0!=(lenStr&0x01)) //str must be an even number of characters
        return 0;

    for(i=0; i<lenStr; i++)
    {
       if((str[i]<'0' || str[i]>'9')&&
          (str[i]<'a' || str[i]>'f')&&
	  (str[i]<'A' || str[i]>'F'))
       {
           return 0;
       }
    }

    return 1;
}
