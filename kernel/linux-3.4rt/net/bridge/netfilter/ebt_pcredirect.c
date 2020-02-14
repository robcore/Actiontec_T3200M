/*
 * Description: EBTables change DNS Option extension kernelspace module.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netfilter.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_pcredirect.h>


#include <linux/ip.h>
#include <linux/udp.h>
#include <net/checksum.h>
#include <linux/in6.h>
#ifdef GPL_CODE_CONTROL_LAYER
#include <linux/ipv6.h>
#include <net/ip6_checksum.h>
#endif

static void ebt_pcredirect_modify_ip(const struct ebt_pcredirect_info *info,struct sk_buff *skb)
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
	//as there dns reponse may have A for request AAAA,still need to check the A address and redirect it.
        while((ntohs(*plen) != 16 || ntohs(*(plen-4)) != 28) &&( ntohs(*plen) !=4 || ntohs(*(plen-4)) != 1))
        {
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) return ;
            plen = (__u16 *)purl;
        }
        while ((ntohs(*plen) == 16 && ntohs(*(plen-4)) == 28)
		||(ntohs(*plen) == 4 && ntohs(*(plen-4))==1)) 
        {
            pip=(__be32 *)(purl+2);
	    if(ntohs(*plen) == 16 && ntohs(*(plen-4)) == 28)
	    {
                *pip=(__be32)(info->redirect_ip6.s6_addr32[0]);
                *(pip+1)=(__be32)(info->redirect_ip6.s6_addr32[1]);
                *(pip+2)=(__be32)(info->redirect_ip6.s6_addr32[2]);
                *(pip+3)=(__be32)(info->redirect_ip6.s6_addr32[3]);
	    }
	    else // when AAAA response with A
		*pip=(__be32)(info->redirect_ip.s_addr);
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
	//as there dns reponse may have A for request AAAA,still need to check the A address and redirect it.
        while((ntohs(*plen) != 4 || ntohs(*(plen-4)) != 1) && (ntohs(*plen) != 16 || ntohs(*(plen-4)) != 28))
        {
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) return ;
            plen = (__u16 *)purl;
        }
        while( (ntohs(*plen) == 4 && ntohs(*(plen-4)) == 1)
		||(ntohs(*plen) == 16 && ntohs(*(plen-4)) == 28))
        {
            pip=(__be32 *)(purl+2);
	    if(ntohs(*plen) == 4 && ntohs(*(plen-4)) == 1)
	        *pip=(__be32)(info->redirect_ip.s_addr);
	    else//when A response with AAAA
	    {
                *pip=(__be32)(info->redirect_ip6.s6_addr32[0]);
                *(pip+1)=(__be32)(info->redirect_ip6.s6_addr32[1]);
                *(pip+2)=(__be32)(info->redirect_ip6.s6_addr32[2]);
                *(pip+3)=(__be32)(info->redirect_ip6.s6_addr32[3]);
	    }
            purl += 12 + ntohs(*plen);
            if((purl + 5) >= (unsigned char *)skb->tail) break;
            plen = (__u16 *)purl;
        }
    }
    /*udp checksum here*/
    udph->check = 0;
#ifdef GPL_CODE_CONTROL_LAYER
    if (isIpv6)
    {
        if (i6h != NULL)
            udph->check = csum_ipv6_magic(&i6h->saddr, &i6h->daddr,
                    ntohs(udph->len),IPPROTO_UDP, csum_partial(udph, ntohs(udph->len), 0));
    }
    else
#endif
    {
        udph->check = csum_tcpudp_magic(ih->saddr, ih->daddr, ntohs(udph->len), IPPROTO_UDP,csum_partial(udph, ntohs(udph->len), 0));
    }
    //After Modify IP of dns response to redirected IP and recalculate udp check, some function will recalcute incorrect UDP checksum later which URL redirect function can't work well. We didn't find the root cause. Richard Diao provided one workaround which bypass the recalculation of the driver.	
    if(skb)
        skb->ip_summed=CHECKSUM_NONE;	
   /*ip checksum here*/
#ifdef GPL_CODE_CONTROL_LAYER
    if (!isIpv6)
#endif
    {
        ih->check = 0;
        ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);
    }
	
    return ;
}

static unsigned int
ebt_pcredirect_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct ebt_pcredirect_info *info = par->targinfo;

	if (!skb_make_writable(skb, 0))
		return EBT_DROP;

	if (info->redirect_ip.s_addr)
	{
		ebt_pcredirect_modify_ip(info, skb);
	}
	return info->target;
}

static int ebt_pcredirect_tg_check(const struct xt_tgchk_param *par)
{
	const struct ebt_pcredirect_info *info = par->targinfo;

	if (BASE_CHAIN && info->target == EBT_RETURN)
		return -EINVAL;

	if (INVALID_TARGET)
		return -EINVAL;

	return 0;
}

static struct xt_target ebt_pcredirect_tg_reg __read_mostly = {
	.name		= EBT_PCREDIRECT_TARGET,
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.target		= ebt_pcredirect_tg,
	.checkentry	= ebt_pcredirect_tg_check,
	.targetsize	= XT_ALIGN(sizeof(struct ebt_pcredirect_info)),
	.me		= THIS_MODULE,
};

static int __init ebt_pcredirect_init(void)
{
	return xt_register_target(&ebt_pcredirect_tg_reg);
}

static void __exit ebt_pcredirect_fini(void)
{
	xt_unregister_target(&ebt_pcredirect_tg_reg);
}

module_init(ebt_pcredirect_init);
module_exit(ebt_pcredirect_fini);
MODULE_DESCRIPTION("Ebtables: PCREDIRECT option modification");
MODULE_LICENSE("GPL");
