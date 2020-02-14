/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H

#if defined(GPL_CODE_WP) && defined(GPL_CODE_DHCP_LEASE)
#include "aei_cms.h"
#endif

#if defined(GPL_CODE)
#include "packet.h"
#endif

#if defined(GPL_CODE_DHCP_LEASE)
#define UPDATE_DHCP_LEASE_TIMEOUT       60      /* second */
#define SAVE_DHCP_LEASE_TIMEOUT         3600    /* second */
#endif

struct dhcpOfferedAddr {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;	/* network order */
	u_int32_t expires;	/* host order */
	char hostname[64];
	// BRCM
#if defined(GPL_CODE)
    int icon;                   /* get device identifier according to option 12/60 */
#endif
#if defined(GPL_CODE_DHCP_LEASE) || defined(GPL_CODE) 
    u_int32_t is_stb;
#endif
#if defined(GPL_CODE) || defined(GPL_CODE) 
    char clientid[256];         /* Client Identifier DHCP option (Option 61) */
#endif
	char vendorid[256]; /* vendor class id, option 60, rfc 2132, max length is 255.  We add 1 for the null. */
	char classid[256];  /* user class id, option 77, rfc 3004, max length is 255.  We add 1 for the null. */
   char oui[8];
   char serialNumber[64];
   char productClass[64];
#if defined(GPL_CODE_WP) && defined(GPL_CODE_DHCP_LEASE)
    u_int32_t isWP;
    char WPProductType[AEI_WP_PRODUCT_TYPE_LEN];
    char WPFirmwareVersion[AEI_WP_FIRMWARE_LEN];
    char WPProtocolVersion[AEI_WP_PROTOCOL_LEN]; 
#if defined(GPL_CODE_WP_AUTO_CONFIG)
    char WPCap[AEI_WP_CAP_LEN];
#endif
#endif
};


void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr);
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease);
int lease_expired(struct dhcpOfferedAddr *lease);
int lease_time_remaining(const struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *oldest_expired_lease(void);
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr);
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr);
u_int32_t find_address(int check_expired, u_int8_t *chaddr);
#if defined(GPL_CODE_DHCPD_VENDOR_POOL)
u_int32_t find_address_vendorid(int check_expired,u_int8_t *chaddr);
#endif
#if defined(GPL_CODE) || defined(GPL_CODE)
u_int32_t AEI_find_address(void);
#endif
struct iface_config_t *find_iface_by_ifname(const char *name);
int check_ip(u_int32_t addr, u_int8_t *chaddr);
void adjust_lease_time(long delta);
#if defined(GPL_CODE)
int AEI_is_vlan_vendor_equipped(struct dhcpMessage *oldpacket, char* buf);
int AEI_is_vendor_equipped(struct dhcpMessage *oldpacket, char* buf);
extern int AEI_wstrcmp(const char *pat, const char *str);
#endif
#if defined(GPL_CODE_DHCP_AUTO_RESERVATION)
u_int32_t AEI_find_rs_staticaddress(void);
#endif

#endif
