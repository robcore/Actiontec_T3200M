/* DoS matching match for iptables
 *
 * (C) 2013 Logan Guo <lguo@actiontec.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/ip.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter/x_tables.h>

MODULE_AUTHOR("Logan Guo <lguo@actiontec.com>");
MODULE_DESCRIPTION("Xtables: dos-based matching");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_dos");

#if 0
#define SPARQ_LOG       printk
#else
#define SPARQ_LOG(format, args...)
#endif

static bool
dos_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
    struct iphdr *iph = ip_hdr(skb);
    SPARQ_LOG("src ip:%x\n", iph->saddr);
    SPARQ_LOG("dst ip:%x\n", iph->daddr);
    
    if (iph->saddr== iph->daddr)
        return true;
    else
        return false;
}

static struct xt_match xt_dos_mt_reg __read_mostly = {
    .name       = "dos",
    .family     = NFPROTO_IPV4,
    .match      = dos_mt,
    .me         = THIS_MODULE,
};

static int __init dos_mt_init(void)
{
    return xt_register_match(&xt_dos_mt_reg);
}

static void __exit dos_mt_exit(void)
{
    xt_unregister_match(&xt_dos_mt_reg);
}

module_init(dos_mt_init);
module_exit(dos_mt_exit);
