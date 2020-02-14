#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_WEB_LEN	40
#define MAX_FOLDER_LEN	56
#define MAX_LIST_NUM	100

#if defined(GPL_CODE)
#define URL_COUNT 100
#define ENTRY_SIZE 256
#define LOG_TIMEOUT 10
#endif

typedef struct _URL{
	char website[MAX_WEB_LEN];
	char folder[MAX_FOLDER_LEN];
#if defined(GPL_CODE)
	char lanIP[16];
#endif
	struct _URL *next;
}URL, *PURL;

PURL purl = NULL;

#if defined(GPL_CODE_SUPPORT_PARENT_CONTROL)
#define MAX_MAC_LEN 128
#define MAX_DAY_LEN 32
#define MAX_ARP_LEN 128
typedef struct _URL_P{
        int  isAllow;
	char website[MAX_WEB_LEN];
	char folder[MAX_FOLDER_LEN];
	char lanMac[MAX_MAC_LEN];
        int isAlways;
        int startTime;
        int endTime;
        char days[MAX_DAY_LEN];
	struct _URL_P *next;
}URL_P, *PURL_P;

PURL_P purl_p = NULL;
char hostinfos[512]={0};
#endif


unsigned int list_count = 0;

//const char list_to_open[] = "/dan/url_list";

//extern int get_url_info();
//extern void add_entry(char *, char *);

