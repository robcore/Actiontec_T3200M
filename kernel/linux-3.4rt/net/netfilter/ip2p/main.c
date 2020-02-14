/*
 * $Id: main.c,v 1.13 2004/03/06 22:30:00 liquidk Exp $
 *
 * ipt_p2p kernel match module.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/version.h>

#ifdef CONFIG_MODVERSIONS
	#include <linux/modversions.h>
#endif

#include "ipt_p2p.h"

MODULE_AUTHOR("Filipe Almeida <filipe@rnl.ist.utl.pt>");
MODULE_DESCRIPTION("IP tables P2P match module");
MODULE_LICENSE("GPL");

/* WARNING: The return value differs from the rest of the match_ functions. */
int match_http(const unsigned char *data,
               const unsigned char *end);

int match_edonkey(const unsigned char *data,
                  const unsigned char *end);

int match_dc(const unsigned char *data,
             const unsigned char *end);

int match_bittorrent(const unsigned char *data,
                     const unsigned char *end);

static bool
match_selected(const struct ipt_p2p_info *pinfo,
               const unsigned char *data,
               const unsigned char *end)
{
	if (pinfo->proto & IPT_P2P_PROTO_FASTTRACK ||
		pinfo->proto & IPT_P2P_PROTO_GNUTELLA ||
		pinfo->proto & IPT_P2P_PROTO_OPENFT)
	{
		int proto;

		/* Returns the protocol that matched, or zero if none of the
		   supported protocols were matched. */
		proto = match_http(data, end);

		if (proto != 0)
		{
			if ((pinfo->proto & proto) != 0)
				return 1;
		}
	}

	if (pinfo->proto & IPT_P2P_PROTO_EDONKEY)
		if (match_edonkey(data, end)) return 1;

	if (pinfo->proto & IPT_P2P_PROTO_BITTORRENT)
		if (match_bittorrent(data, end)) return 1;

	if (pinfo->proto & IPT_P2P_PROTO_DIRECT_CONNECT)
		if (match_dc(data, end)) return 1;

	return 0;
}

static bool
match(const struct sk_buff *skb, struct xt_action_param *par)
/*match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
      //const void *hdr,
      //u_int16_t datalen,
#endif /* Linux < 2.6.0 */

      //int *hotdrop)
{
	const struct ipt_p2p_info *pinfo = par->matchinfo;
	//const struct iphdr *iph = skb->nh.iph;
	const struct iphdr *iph = ip_hdr(skb);
	const struct tcphdr *tcph;
	const unsigned char *data;
	const unsigned char *end;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	int datalen;
	datalen = skb->len - (iph->ihl<<2);
#endif /* Linux >= 2.6.0 */

	/* We only support TCP-based matches currently. */
	if (!iph || iph->protocol != IPPROTO_TCP) return 0;

	/* Access the application-layer data from the packet */
	//tcph = (void *)skb->nh.iph + skb->nh.iph->ihl*4;
	tcph = (void *)iph + iph->ihl*4;
	data = (const unsigned char *) tcph + tcph->doff * 4;
	end = data + datalen - tcph->doff * 4;

	/* Handle the requested protocol(s). */
	return match_selected(pinfo, data, end);
}

static int
checkentry(const struct xt_mtchk_param *par)
/*checkentry(const char *tablename,
           const struct ipt_ip *ip,
           void *matchinfo,
           unsigned int matchsize,
           unsigned int hook_mask)*/
{
	if (par->match->matchsize != XT_ALIGN(sizeof(struct ipt_p2p_info)))
		return 1;

	return 0;
}

static struct xt_match p2p_match __read_mostly = {
	.name        = "p2p",
    .family     = NFPROTO_IPV4,
	.match       = match,
	.checkentry  = checkentry,
	.matchsize  = XT_ALIGN(sizeof(struct ipt_p2p_info)),
	.me          = THIS_MODULE,
};

static int __init init(void)
{
	printk(KERN_INFO "iptables-p2p %s initialized\n", IPT_P2P_VERSION);
	return xt_register_match(&p2p_match);
}

static void __exit fini(void)
{
	xt_unregister_match(&p2p_match);
	printk(KERN_INFO "iptables-p2p %s removed\n", IPT_P2P_VERSION);
}

module_init(init);
module_exit(fini);
