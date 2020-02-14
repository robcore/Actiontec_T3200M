#ifndef __AEI_UTILES_H__
#define __AEI_UTILES_H__
#include "mdm_object.h"
#ifdef GPL_CODE_LOCKOUT_AGAINST_BRUTE_FORCE_ATTACKS
UBOOL8 isCliLocked(UINT32 timeout);
CmsRet saveCliLockInfo(UINT32 max_retry, UINT32 curr_retry, UINT32 timeout);
CmsRet getCliLockInfo(UINT32* max_retry, UINT32* curr_retry, UINT32* timeout);
void saveCliLockUptime(char *appname);
#endif
#if defined(GPL_CODE_QUANTENNA_SUPPORT)
#define QCSAPI_TARGET_IP "169.254.1.2"
#endif

#if defined(GPL_CODE_DSL_STATE_GUI_ACCESS_NO_AUTH)
#define AEI_DSL_CLEAR_LOG_FILE "/var/aei_dsl_clear_log.txt"
#endif

int AEI_get_value_by_file(char *file, int size, char *value);
UINT16 AEI_get_interface_mtu(char *ifname);
void AEI_createFile(char *filename, char *content);
int AEI_removeFile(char *filename);
int AEI_isFileExist(char *filename);
int AEI_get_mac_addr(char *ifname, char *mac);
int AEI_convert_space(char *src,char *dst);
int AEI_convert_spec_chars(char *src,char *dst);
char* AEI_SpeciCharEncode(char *s, int len);
char *AEI_getGUIStrValue(char *v_in, char *v_out, int v_out_len);
char *AEI_getGUIStrValue2(char *v_in, int v_in_len);
pid_t* find_pid_by_name( char* pidName);
int AEI_GetPid(char * command);
int AEI_get_wifi_idx_from_msg(char *msg, int index);
char* AEI_getCmdOutPut(char* command, char* value, int len);
int AEI_isHexStringValid(char *str);
#if defined(GPL_CODE_CONFIG_JFFS) && defined(GPL_CODE_C1000A)
CmsRet AEI_writeDualPartition(char *imagePtr, UINT32 imageLen, void *msgHandle, int partition);
#endif
#if  defined(GPL_CODE_DETECT_LOG)
typedef enum
{
    AEI_WAN_ETH,
    AEI_WAN_ADSL,
    AEI_WAN_VDSL,
}AEIWanDevType; 
#endif
#if defined(GPL_CODE)
int AEI_save_syslog();
#endif
#endif
