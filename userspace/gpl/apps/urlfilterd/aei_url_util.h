#ifndef __AEI_URL_UTIL_H__
#define __AEI_URL_UTIL_H__

#define WEB_PORT 80
#define capURLFile "/var/captive_url"
#define capAllowListFile  "/var/captive_allowlist"
#define capAllowDomainFile  "/var/captive_allowlist_domain"
#define oneTimeCapURLFile "/var/oneTimeRedirectURL"

int AEI_checkCaptiveAllowList(char *allowList, __be32 ip);
int AEI_send_redirect (struct nfq_q_handle *qh, int id, struct nfq_data * payload, char *capurl);
void AEI_get_lan_ip(char *addr);
void AEI_getCaptiveAllowList();
void AEI_getCaptiveURLandIPAddr(char *fileName, char *url, char *ipAddr, int *flag);
#ifdef GPL_CODE_CAPTIVEPORTAL_DOMAIN
char *AEI_getdomain(char *data, char *url);
int AEI_checkCaptiveAllowDomain(char *allowDomain, char* host);
void AEI_getCaptiveAllowDomain();
#endif
#if defined (AEI_WLAN_URL_REDIRECT)
void AEI_processSSID234UrlRedirect(char *mac,char *url,int size,char *ifname,char *match);
void AEI_urltrim(char *url_redirect);
int AEI_SendPacketWithDestMac(char *data, int len, unsigned char *dmac,char *brname);
#else
int AEI_SendPacketWithDestMac(char *data, int len, unsigned char *dmac);
#endif
#if defined(GPL_CODE)
CmsRet AEI_send_msg_to_set_oneTimeRedirectURLFlag();
#endif

#if defined(GPL_CODE_FRONTIER_WIZARD)
int AEI_getWizardFlag();
#endif
#endif
