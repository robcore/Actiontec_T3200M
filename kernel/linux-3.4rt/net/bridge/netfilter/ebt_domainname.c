/*
 * Description: EBTables domainname name match extension kernelspace module.
 * Authors: yirun wang <ywang@actiontec.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_domainname.h>

#include <linux/ip.h>
#include <net/ip.h>
#include <linux/in.h>
#include <linux/module.h>
#ifdef GPL_CODE_CONTROL_LAYER
#include <linux/ipv6.h>
#endif


#include <linux/types.h>
#include <linux/ctype.h>


#define MODULE_VERS "0.1"

MODULE_AUTHOR("yirun wang <ywang@actiontec.com>");
MODULE_DESCRIPTION("Ebtables: DNS domain name tag match");
MODULE_LICENSE("GPL");

struct tcpudphdr {
    __be16 src;
    __be16 dst;
};

//QA-Bug #232949: [GUI]Pause Internet func don't work after dut reboot. This may be caused by the domain name in whitelist having more than one IP Address;
//the IP Address resolved by DNS are different in different time(such as, www.zhihu.com); then, the IP Address of the domain name parsed by the LAN PC after
//reboot DUT is different from the IP Address in the whitelist IP rules which is parsed when the whitelist is set;
//In order to fix this issue. dynamically cache 100 newest whitelist ips here.
#define MAX_WHITELIST 100

typedef struct {
    char domainname[DOMAINNAME_SIZE];
    __be32 ipaddress4;
    __be32 ipaddress6[4];
} record;

record whitelist[MAX_WHITELIST];
int currentIndex=0;


static int AEI_wstrcmp(const char *pat, const char *str)
{
    const char *p = NULL, *s = NULL;
    do {
        if (*pat == '*') {
            for (; *(pat + 1) == '*'; ++pat) ;
            p = pat++;
            s = str;
        }
        if (*pat == '?' && !*str)
            return -1;

        if (*pat != '?' && toupper(*pat) != toupper(*str)) {
            if (p == NULL)
                return -1;
            pat = p;
            str = s++;
        }
    } while (*pat && ++pat && *str++);
    for (; *pat == '*'; ++pat) ;
    return *pat;
}

//we need cover *.telus.*,*telus*,www.telus.com,telus.com...
static int AEI_domainname_cmp(const char *pat, const char *str)
{
    const char *p=pat;
    const char *s=str;
    if(*p != '*')
    { 
        if(tolower(*p)=='w' && tolower(*(p+1))=='w' && tolower(*(p+2))=='w' && *(p+3)=='.') p = p+4;
        if(tolower(*s)=='w' && tolower(*(s+1))=='w' && tolower(*(s+2))=='w' && *(s+3)=='.') s = s+4;
    }
    return AEI_wstrcmp(p,s);
}

bool
ebt_domainname_whitelist_mt6(const char *domainname, struct in6_addr daddr)
{
    int i=0;
    int count=MAX_WHITELIST;
    int start=(currentIndex==0?MAX_WHITELIST-1:currentIndex-1);
    if(domainname && strlen(domainname)!=0)
    {
        for (i=start;count>0;i=(i==0?MAX_WHITELIST-1:i-1),count--) 
        {

            if(!AEI_domainname_cmp(domainname,whitelist[i].domainname) &&
               whitelist[i].ipaddress6[0] == daddr.s6_addr32[0] && 
               whitelist[i].ipaddress6[1] == daddr.s6_addr32[1] && 
               whitelist[i].ipaddress6[2] == daddr.s6_addr32[2] && 
               whitelist[i].ipaddress6[3] == daddr.s6_addr32[3]) 
            return true;
        }
    } else
    {
        for (i=start;count>0;i=(i==0?MAX_WHITELIST-1:i-1),count--) 
        {
            if(whitelist[i].ipaddress6[0] == daddr.s6_addr32[0] && 
               whitelist[i].ipaddress6[1] == daddr.s6_addr32[1] && 
               whitelist[i].ipaddress6[2] == daddr.s6_addr32[2] && 
               whitelist[i].ipaddress6[3] == daddr.s6_addr32[3])   
            return true;    
        }
        
    }
    return false;
}


static bool
ebt_domainname_whitelist_mt4(const char *domainname, __be32 ip)
{
    int i=0;
    int count=MAX_WHITELIST;
    int start=(currentIndex==0?MAX_WHITELIST-1:currentIndex-1);
    if(domainname && strlen(domainname)!=0)
    {
        for (i=start;count>0;i=(i==0?MAX_WHITELIST-1:i-1),count--) 
        {

            if(!AEI_domainname_cmp(domainname,whitelist[i].domainname) && 
                whitelist[i].ipaddress4 == ip) return true;
        }
    } else
    {
        for (i=start;count>0;i=(i==0?MAX_WHITELIST-1:i-1),count--) 
            if(whitelist[i].ipaddress4 == ip) return true;
        
    }
    return false;
}

static void ebt_save_domainname_ip_to_whitelist(const struct sk_buff *skb, struct xt_action_param *par,const char* domainname)
{

    struct iphdr *ih;
    struct iphdr _iph;
#ifdef GPL_CODE_CONTROL_LAYER
    struct ipv6hdr *i6h = NULL;
    struct ipv6hdr _ip6h;
    struct udphdr _udph;
#endif
    struct udphdr *udph;
    unsigned char *purl;
    __be32 *pip;
    __u16 *plen;
#ifdef GPL_CODE_CONTROL_LAYER
    bool isIpv6 = false;
#endif

    ih = skb_header_pointer(skb, 0, sizeof(_iph), &_iph);
    if (ih == NULL)
        return ;
    if (ih->version != 4)
    {//IPv6 TODO
#ifdef GPL_CODE_CONTROL_LAYER
        i6h = skb_header_pointer(skb, 0, sizeof(_ip6h), &_ip6h);
        if (i6h == NULL || i6h->version != 6)
            return;

        udph = skb_header_pointer(skb, sizeof(_ip6h), sizeof(_udph), &_udph );
        if (udph == NULL)
            return;

        isIpv6 = true;
#else
        return ;
#endif
    }
    else
    {
        udph = (struct udphdr *) ((unsigned char*)ih + (ih->ihl << 2) );
        if( udph == NULL)
            return ;
    }
     
    purl = (unsigned char *)udph;
    purl += 0x15;
    while (purl < (unsigned char *)skb->tail  && !(*(purl)==0x0 && (*(purl+1)==0x01/*IPv4*/ || *(purl+1)==0x1c/*IPv6*/) && *(purl+2)==0x0 && *(purl+3)==0x01))
    {
        purl++;
        if( purl >= (unsigned char *)skb->tail ) return ;
    }
    if(*(purl+1)==0x1c)/*IPv6*/
    {
        purl += 14;
        if( (purl+5) >= (unsigned char *)skb->tail ) return ;
        plen = (__u16 *)purl;
        if (plen >= (__u16 *)skb->tail) return ;
        while(ntohs(*plen) != 16 || ntohs(*(plen-4)) != 28)
        {
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) return ;
            plen = (__u16 *)purl;
        }
        while (ntohs(*plen) == 16 && ntohs(*(plen-4)) == 28)
        {
            pip=(__be32 *)(purl+2);
            if(domainname && strlen(domainname) > 0) strcpy(whitelist[currentIndex].domainname,domainname);
            whitelist[currentIndex].ipaddress6[0] = *pip;
            whitelist[currentIndex].ipaddress6[1] = *(pip+1);
            whitelist[currentIndex].ipaddress6[2] = *(pip+2);
            whitelist[currentIndex].ipaddress6[3] = *(pip+3);
            whitelist[currentIndex].ipaddress4=0;
            if((++currentIndex) >= MAX_WHITELIST) currentIndex=0;
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) break;
            plen = (__u16 *)purl;
        }
    }else/*IPv4*/
    {
        purl += 14;
        if( (purl+5) >= (unsigned char *)skb->tail ) return ;
        plen = (__u16 *)purl;
        if (plen >= (__u16 *)skb->tail) return ;
        while(ntohs(*plen) != 4 || ntohs(*(plen-4)) != 1)
        {
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) return ;
            plen = (__u16 *)purl;
        }
        while (ntohs(*plen) == 4 && ntohs(*(plen-4)) == 1)
        {
            pip=(__be32 *)(purl+2);
            if(domainname && strlen(domainname) > 0) strcpy(whitelist[currentIndex].domainname,domainname);
            whitelist[currentIndex].ipaddress4 = *pip;
            whitelist[currentIndex].ipaddress6[0] = 0;
            whitelist[currentIndex].ipaddress6[1] = 0;
            whitelist[currentIndex].ipaddress6[2] = 0;
            whitelist[currentIndex].ipaddress6[3] = 0;
            if((++currentIndex) >= MAX_WHITELIST) currentIndex=0;
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) break;
            plen = (__u16 *)purl;
        }
   }

    return ;
}


static bool
ebt_domainname_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
    const struct ebt_domainname_info *info = par->matchinfo;
    const struct iphdr *ih;
    struct iphdr _iph;
#ifdef GPL_CODE_CONTROL_LAYER
    const struct ipv6hdr *i6h;
    struct ipv6hdr _ip6h;
#endif
    int i;

    const struct tcpudphdr *pptr;
    unsigned char *purl;
    unsigned char temp_url[DOMAINNAME_SIZE];
    struct tcpudphdr _ports;
    memset(temp_url,0,DOMAINNAME_SIZE);

    ih = skb_header_pointer(skb, 0, sizeof(_iph), &_iph);
    if (ih == NULL)
        return false;
        
    if (ih->version != 4)
    {//IPv6 TODO
#ifdef GPL_CODE_CONTROL_LAYER
        i6h = skb_header_pointer(skb, 0, sizeof(_ip6h), &_ip6h);
        if (i6h == NULL || i6h->version != 6)
            return false;
        if(info->matchWhiteList)
        {
            return ebt_domainname_whitelist_mt6(info->domainname,i6h->daddr);
        }
        pptr = skb_header_pointer(skb, sizeof(_ip6h), sizeof(_ports), &_ports);
        if (pptr == NULL)
            return false;
#else
        return false;
#endif
    }
    else
    {
        if(info->matchWhiteList)
        {
            return ebt_domainname_whitelist_mt4(info->domainname,ih->daddr);
        }

        pptr = skb_header_pointer(skb, ih->ihl*4, sizeof(_ports), &_ports);
        if( pptr == NULL)
            return false;
    }

    purl = (unsigned char *)pptr;
    purl += 0x15;
    for (i=0; i<DOMAINNAME_SIZE;i++)
    {
        if(*(purl+i) == 0x00) break;
        if(*(purl+i) <  0x20) 
        {
            temp_url[i]='.';
            continue;
        }
        temp_url[i] = *(purl+i);      
    }
    if(AEI_domainname_cmp(info->domainname,temp_url)) return false;
    if(info->saveWhiteList)
    {
        ebt_save_domainname_ip_to_whitelist(skb,par,temp_url);
    }

	return true;
}

static void ebt_clean_domainname_whitelist(const struct xt_mtdtor_param *par)
{
    memset(whitelist, 0, sizeof( record ) * MAX_WHITELIST);
    currentIndex=0;
}

static int ebt_domainname_mt_check(const struct xt_mtchk_param *par)
{
//	struct ebt_vlan_info *info = par->matchinfo;
//	const struct ebt_entry *e = par->entryinfo;

	/* Is it DNS query frame checked? */
//		return -EINVAL;

	return 0;
}

static struct xt_match ebt_domainname_mt_reg __read_mostly = {
	.name		= "domainname",
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.match		= ebt_domainname_mt,
        .destroy        = ebt_clean_domainname_whitelist,
	.checkentry	= ebt_domainname_mt_check,
	.matchsize	= sizeof(struct ebt_domainname_info),
	.me		= THIS_MODULE,
};

static int __init ebt_domainname_init(void)
{
	pr_debug("ebtables DNS domainname name extension module v" MODULE_VERS "\n");
        memset(whitelist, 0, sizeof( record ) * MAX_WHITELIST);
	return xt_register_match(&ebt_domainname_mt_reg);
}

static void __exit ebt_domainname_fini(void)
{
	xt_unregister_match(&ebt_domainname_mt_reg);
}

module_init(ebt_domainname_init);
module_exit(ebt_domainname_fini);
