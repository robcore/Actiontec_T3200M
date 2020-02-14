/* options.h */
#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "packet.h"
#include "leases.h"

#define TYPE_MASK	0x0F

enum {
	OPTION_IP=1,
	OPTION_IP_PAIR,
	OPTION_STRING,
	OPTION_BOOLEAN,
	OPTION_U8,
	OPTION_U16,
	OPTION_S16,
	OPTION_U32,
	OPTION_S32
};

#define OPTION_LIST	0x80

struct dhcp_option {
	char name[10];
	char flags;
	unsigned char code;	// to support 6rd option (212), 'char' => 'unsigned char'
};

extern struct dhcp_option options[];
extern int option_lengths[];

unsigned char *get_option(struct dhcpMessage *packet, int code);
int end_option(unsigned char *optionptr);
int add_option_string(unsigned char *optionptr, unsigned char *string);
int add_simple_option(unsigned char *optionptr, unsigned char code, u_int32_t data);
struct option_set *find_option(struct option_set *opt_list, char code);
void attach_option(struct option_set **opt_list, struct dhcp_option *option, char *buffer, int length);

// brcm
extern char session_path[];

// brcm
int createVIoption(int type, char *VIinfo);
#ifdef GPL_CODE
int CreateOption125(int type, char *oui, char *sn, char *prod, char *mn, char *VIinfo);
#else
int CreateOption125(int type, char *oui, char *sn, char *prod, char *VIinfo);
#endif
int CreateClntId(char *iaid, char *duid, char *out_op);
int saveVIoption(char *option, struct dhcpOfferedAddr *lease);

#if defined(GPL_CODE_WP) && defined(GPL_CODE_DHCP_LEASE)
unsigned char *get_subOption(struct dhcpMessage *packet, int code, const char * subtypeStr);
unsigned char AEI_InitLeaseWP(struct dhcpMessage *packet , struct dhcpOfferedAddr *lease);
#endif

#if defined(GPL_CODE_DHCP_LEASE)
void getClientIDOption(struct dhcpMessage *packet, struct dhcpOfferedAddr *lease);
#if defined(GPL_CODE)
void AEI_DefaultSTBHostName(struct dhcpMessage *packet , struct dhcpOfferedAddr *lease);
#endif
#endif

#define _PATH_RESOLV	 "/var/fyi/sys/dns"
#define _PATH_GW	 "/var/fyi/sys/gateway"
#define _PATH_SYS_DIR	 "/var/fyi/sys"

#define _PATH_WAN_DIR	"/proc/var/fyi/wan"
#define _PATH_MSG	"daemonstatus"
#define _PATH_IP	"ipaddress"
#define _PATH_MASK	"subnetmask"
#define _PATH_PID	"pid"

#ifdef GPL_CODE
#define VENDOR_DEVICE_ACS_URL_SUBCODE  1
#define VENDOR_DEVICE_PRO_CODE_SUBCODE  2
#define VENDOR_DEVICE_WAIT_INTVAL_SUBCODE 3
#define VENDOR_DEVICE_RETRY_INTVAL_SUBCODE 4
#endif
#if defined(GPL_CODE_AMT)
#define VENDOR_DEVICE_USERNAME_SUBCODE 111
#define VENDOR_DEVICE_PASSWORD_SUBCODE 113
#endif

/* option header: 2 bytes + subcode headers (6) + option data (64*2+6);
   these are TR111 option data.    Option can be longer.  */
#if defined(GPL_CODE)
#define VENDOR_BRCM_ENTERPRISE_NUMBER     (htonl(3561))
#define VENDOR_IDENTIFYING_OPTION_NAME   "dslforum.org"
#else
#define VENDOR_BRCM_ENTERPRISE_NUMBER     4413
#endif
#define VENDOR_DSLFORUM_ENTERPRISE_NUMBER 3561
#define VENDOR_ENTERPRISE_LEN             4    /* 4 bytes */
#define VENDOR_IDENTIFYING_INFO_LEN       142
#define VENDOR_IDENTIFYING_OPTION_CODE    125
#define VENDOR_OPTION_CODE_OFFSET         0
#define VENDOR_OPTION_LEN_OFFSET          1
#define VENDOR_OPTION_ENTERPRISE_OFFSET   2
#define VENDOR_OPTION_DATA_OFFSET         6
#define VENDOR_OPTION_DATA_LEN            1
#define VENDOR_OPTION_SUBCODE_LEN         1
#define VENDOR_SUBCODE_AND_LEN_BYTES      2
#define VENDOR_DEVICE_OUI_SUBCODE            1
#define VENDOR_DEVICE_SERIAL_NUMBER_SUBCODE  2
#define VENDOR_DEVICE_PRODUCT_CLASS_SUBCODE  3
#define VENDOR_GATEWAY_OUI_SUBCODE           4
#define VENDOR_GATEWAY_SERIAL_NUMBER_SUBCODE 5
#define VENDOR_GATEWAY_PRODUCT_CLASS_SUBCODE 6
#ifdef GPL_CODE
#define VENDOR_MODEL_NAME_SUBCODE 7
#endif
#if defined(GPL_CODE_FEATURE_GATEWAY_CAPABILITY)
#define VENDOR_GATEWAY_CAPABILITY_SUBCODE  101
#endif
#define VENDOR_IDENTIFYING_FOR_GATEWAY       1
#define VENDOR_IDENTIFYING_FOR_DEVICE        2
#if defined(GPL_CODE_FEATURE_GATEWAY_CAPABILITY)
#define VENDOR_IDENTIFYING_FOR_GATEWAY_CAPABILITY    4
#endif
#define VENDOR_GATEWAY_OUI_MAX_LEN           6
#define VENDOR_GATEWAY_SERIAL_NUMBER_MAX_LEN 64 
#define VENDOR_GATEWAY_PRODUCT_CLASS_MAX_LEN 64

#endif
