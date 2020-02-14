#define _GNU_SOURCE
#define PROFILE_LOG_FILENAME "/var/profileLog"
#define PROFILE_PID_FILENAME "/var/pidLog"
#define PROFILE_LOG_PUTLOG_FILENAME "/var/putlog/profileLog"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dlfcn.h>
#include <time.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#if defined (CMS_CORE)
#include "cms_lck.h"
#else

#ifndef BUFLEN_64
#define BUFLEN_64  64
#endif

#ifndef BUFLEN_256
#define BUFLEN_256 256
#endif

#endif

enum{
    PROFILE_LOG_DISABLE=0,
    PROFILE_LOG_CMS_LCK_SUPPORT,
    PROFILE_LOG_FULL_SUPPORT,
};


/*
To prevent dead loop cause memory over,add two limitation , 
limit log file size.(max line is 20000).
*/

#define MAX_LINE_NUM 2000
int total_line_num=0;
/*
One case that will cause issue is while loop calling function(s) thousands of times.
calling dladdr thousands of times in a loop is slow so keep a table of most recent calls for lookup
*/
#define MAX_LOOKUP 10

typedef struct {
    void *address;
    const char *ptr_sname;
    const char *ptr_fname;
} record;

record lookup[MAX_LOOKUP]= {[0 ... MAX_LOOKUP -1] = {0,0,0}};
int currentIndex=0;

/* this macro searches backwards and inserts forward on a circular array to simulate a stack
 * need to do a macro because
 * 1. calling dladdr within a function called by __cyg_profile_func_enter/exit will segfault
 * 2. not to incur additional overhead
 */
#define lookup_dladdr(fn,finfo) \
({ \
    int i=0;\
    int count=MAX_LOOKUP;\
    int found=0;\
    for (i=currentIndex;count>0;i=(i==0?MAX_LOOKUP-1:i-1),count--) \
    { \
        if (lookup[i].address)\
        {\
            if(lookup[i].address==fn) \
            { \
                currentIndex = i; \
                finfo.dli_sname=lookup[i].ptr_sname;\
                finfo.dli_fname=lookup[i].ptr_fname;\
                found=1;\
                break; \
            }\
        }\
        else\
        {\
            break;\
        }\
    } \
    if (!found) \
    { \
        if (dladdr(fn,&finfo)) \
        { \
            if (count<MAX_LOOKUP) i=(currentIndex+1)%MAX_LOOKUP;\
            lookup[i].address=fn; \
            lookup[i].ptr_sname=finfo.dli_sname;\
            lookup[i].ptr_fname=finfo.dli_fname;\
            currentIndex = i;\
        } else  \
        { \
            finfo.dli_sname=NULL; \
            finfo.dli_fname=NULL; \
        } \
    }\
}) \

int profileLogLevel=PROFILE_LOG_DISABLE;

static int write_flag=0;
#if defined (CMS_CORE)
static void    *handle=NULL;
static void    *f_oal_lock=NULL;
static void    *f_oal_unlock=NULL;
#endif

/* "killall -18 httpd" or "kill -18 pid" to switch profiling log level for that process(switch rule:
 * cycle from disable profiling to cms lock profiling to full profiling back to disable profiling) 
 * this is non persistent and does not touch datamodel param
 */
void signal_func_trace(__attribute__((unused)) int signum, __attribute__((unused)) siginfo_t* info, __attribute__((unused)) void*ptr)
{
     profileLogLevel=(profileLogLevel==PROFILE_LOG_DISABLE)?PROFILE_LOG_CMS_LCK_SUPPORT:((profileLogLevel==PROFILE_LOG_CMS_LCK_SUPPORT)?PROFILE_LOG_FULL_SUPPORT:PROFILE_LOG_DISABLE);
     printf("@@@@@@set profile log level to %d(0:disable,1:cms lock debug,2:full support)@@@@\n",profileLogLevel);
     fflush(stdout);
}

/* "killall -23 httpd" or "kill -23 pid" to make process release the cms lock 
 *  can be used by another process when it cannot get lock for very long time...
 */
void signal_func_freelck(__attribute__((unused)) int signum, __attribute__((unused)) siginfo_t* info, __attribute__((unused)) void*ptr)
{
#if defined (CMS_CORE)
     printf("@@@@@@releasing lock@@@@@@\n");
     fflush(stdout);
     cmsLck_releaseLock();
#endif
}

void sig_handle_init_trace(void)
{
    struct sigaction action;
    /* sigcont "continue" signal does not look to be used by ssk,httpd or tr69 */
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_func_trace;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCONT, &action, NULL);

    /* sigurg "urgent" signal does not look to be used by ssk,httpd or tr69 */
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_func_freelck;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGURG, &action, NULL);
}





/*
 *  To cover process that does not have datamodel access and yet want to know if it needs to turn
 *  on or off the profiling, use an 2d array in shared memory or RAM file.
 */
#define MAX_EID_COUNT 32
#define PROFILE_EID_SETTINGS "/var/profileEIDSettings"

void setProfileLogLevel(int eid,int i)
{
    int data[MAX_EID_COUNT][2];
    FILE *fp=NULL;
    int j=0;
    memset(data,0,sizeof(data));

    fp = fopen(PROFILE_EID_SETTINGS,"rb");
    if (fp)
    {
        fread(data,sizeof(int),MAX_EID_COUNT*2,fp);
        fclose(fp);
    }

    for (j=0;j<MAX_EID_COUNT;j++)
    {
        if (data[j][0]==0)
        {
           data[j][0]=eid;
           data[j][1]=i;
           break;
        }
        else if (data[j][0]==eid)
        {
           data[j][1]=i;
           break;
        }
    }

    fp = fopen(PROFILE_EID_SETTINGS,"wb");
    if (fp)
    {
        fwrite(data,sizeof(int),MAX_EID_COUNT*2,fp);
        fclose(fp);
    }
}

int getProfileLogLevel(int eid)
{
    int data[MAX_EID_COUNT][2];
    FILE *fp=NULL;
    int j=0;
    memset(data,0,sizeof(data));

    fp = fopen(PROFILE_EID_SETTINGS,"rb");
    if (fp)
    {
        fread(data,sizeof(int),MAX_EID_COUNT*2,fp);
        fclose(fp);
    }

    for (j=0;j<MAX_EID_COUNT;j++)
    {
        if (data[j][0]==eid)
        {
           return data[j][1];
        }
    }

    return 0;
}


inline void aei_write_str_to_profilelog(const char *str)
__attribute__((no_instrument_function));
inline void aei_write_str_to_profilelog(const char *str)
{
    FILE *fp=NULL;
    fp = fopen(PROFILE_LOG_FILENAME,"a");
    if (fp)
    {
        fprintf(fp,"%s\n", str);
        fclose(fp);
        /* first process that gets to MAX_LINE_NUM renames the file,
           but after file is renamed by a process, other processed still
           keep old accounting so file size/length will vary....
           this is very cheap to do and fast so ok for now...
         */
        if(++total_line_num >= MAX_LINE_NUM)
        {
            char newfilename[BUFLEN_64];
            snprintf(newfilename,sizeof(newfilename),"%s%lu",PROFILE_LOG_PUTLOG_FILENAME,time(NULL));
            rename(PROFILE_LOG_FILENAME,newfilename);
            total_line_num=0;
        }
    }
}

inline void aei_write_str_to_pidlog(const char *str)
__attribute__((no_instrument_function));
inline void aei_write_str_to_pidlog(const char *str)
{
    FILE *fp=NULL;
    fp = fopen(PROFILE_PID_FILENAME,"a");
    if (fp)
    {
        fprintf(fp,"%s\n", str);
        fclose(fp);
    }
}

inline void aei_profile_log(const char *flag,void *this_fn, void *call_site)
__attribute__((no_instrument_function));
inline void aei_profile_log(const char *flag,void *this_fn, void *call_site)
{
    Dl_info finfo;
    Dl_info finfo2;
    lookup_dladdr(call_site,finfo2);
    lookup_dladdr(this_fn,finfo);

    char str[BUFLEN_256]={0};
    struct timeval t;
    gettimeofday(&t, NULL);
    if (finfo.dli_sname && finfo2.dli_sname)
    {
        sprintf(str,"%s %s %s %lu.%03lu.%03lu %d", flag,finfo.dli_sname,finfo2.dli_sname,t.tv_sec,t.tv_usec/1000,t.tv_usec%1000,getpid());
    }else if(finfo.dli_sname && !finfo2.dli_sname)
    {
        sprintf(str,"%s %s %p %lu.%03lu.%03lu %d", flag,finfo.dli_sname,call_site,t.tv_sec,t.tv_usec/1000,t.tv_usec%1000,getpid());
    }else if(!finfo.dli_sname && finfo2.dli_sname)
    {
        sprintf(str,"%s %p %s %lu.%03lu.%03lu %d", flag,this_fn,finfo2.dli_sname,t.tv_sec,t.tv_usec/1000,t.tv_usec%1000,getpid());
    }else
    {
        sprintf(str,"%s %p %p %lu.%03lu.%03lu %d", flag,this_fn,call_site,t.tv_sec,t.tv_usec/1000,t.tv_usec%1000,getpid());
    }
    aei_write_str_to_profilelog(str);
}
void
__attribute__ ((constructor))
trace_begin (void)
{
    /* log process pid, name and launch time */

    FILE *fp=NULL;
    char cmdline[BUFLEN_256]={0};
    char cmdlineFile[BUFLEN_256]={0};
    char logString[BUFLEN_256]={0};
    sprintf(cmdlineFile,"/proc/%d/cmdline",getpid());
    fp=fopen(cmdlineFile,"r");
    if(fp)
    {
        struct timeval t;
        gettimeofday(&t, NULL);
        fread(cmdline, BUFLEN_256, 1, fp);
        fclose(fp);
        sprintf(logString,"S %s na %lu.%03lu.%03lu %d",cmdline,t.tv_sec,t.tv_usec/1000,t.tv_usec%1000,getpid());
        aei_write_str_to_pidlog(logString);
    }
    /* if the number of log files (in /var/putlog) > XX, delete the oldest file */
    system("aei_deleteOldestProfileLog.sh 50&");

#if defined (CMS_CORE)
    /* open the needed object */
    handle = dlopen("/lib/libcms_core.so", RTLD_LOCAL | RTLD_LAZY);
    if(handle)
    {
        f_oal_lock = dlsym(handle, "oal_lock");
        f_oal_unlock = dlsym(handle, "oal_unlock");
        dlclose(handle);
    }
#endif
    /* embed a signal handler to allow signals to be sent directly to a process */
    sig_handle_init_trace();
}



void __cyg_profile_func_enter(void *this_fn, void *call_site)
__attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site) {

    if(profileLogLevel != PROFILE_LOG_DISABLE )
    {

        if(profileLogLevel == PROFILE_LOG_FULL_SUPPORT
#if defined (CMS_CORE)
          || this_fn == f_oal_lock
#endif
        )
        {
            write_flag = 1;
        }
        if(write_flag)
        {
            aei_profile_log("E",this_fn,call_site);
        }
    }
} /* __cyg_profile_func_enter */

void __cyg_profile_func_exit(void *this_fn, void *call_site)
__attribute__((no_instrument_function));

void __cyg_profile_func_exit(void *this_fn, void *call_site) {

    if(write_flag && profileLogLevel != PROFILE_LOG_DISABLE)
    {
        if(profileLogLevel == PROFILE_LOG_CMS_LCK_SUPPORT
#if defined (CMS_CORE)
        && this_fn == f_oal_unlock
#endif
        )
        {
            //if release cms lock, delete the file since not locking
            unlink(PROFILE_LOG_FILENAME);
            write_flag=0;
            total_line_num=0;
        } else
        {
            aei_profile_log("X",this_fn,call_site);
        }
    }

} /* __cyg_profile_func_enter */
