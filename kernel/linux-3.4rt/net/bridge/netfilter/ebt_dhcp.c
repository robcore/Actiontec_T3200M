/*
 * Description: EBTables DHCP Option extension kernelspace module.
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
#include <linux/netfilter_bridge/ebt_dhcp.h>

#include <linux/ip.h>
#include <linux/udp.h>
#include <net/checksum.h>

#define MACSTR "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

struct dhcpMessage {
        u_int8_t op;
        u_int8_t htype;
        u_int8_t hlen;
        u_int8_t hops;
        u_int32_t xid;
        u_int16_t secs;
        u_int16_t flags;
        u_int32_t ciaddr;
        u_int32_t yiaddr;
        u_int32_t siaddr;
        u_int32_t giaddr;
        u_int8_t chaddr[16];
        u_int8_t sname[64];
        u_int8_t file[128];
        u_int32_t cookie;
        u_int8_t options[308]; /* 312 - cookie */
};

#define DHCP_PADDING                0x00
#define DHCP_OPTION_MESSAGE_TYPE    0x35
#define DHCP_OPTION_AGENT_OPTIONS   0x52
#define DHCP_END                    0xFF

/* Option 82 Relay Agent Information option subtypes: */
#define RAI_CIRCUIT_ID  1
#define RAI_REMOTE_ID   2

static int ebt_dhcp_construct_option82(const char* circuit_id, const char* remote_id, struct dhcpMessage *packet, unsigned int len)
{
	u_int8_t *op, *sp, *max = NULL;
	unsigned int  optlen = 0, circuit_len = 0, remote_len = 0;

	circuit_len = strlen(circuit_id);
	remote_len = strlen(remote_id);

	if (circuit_len)
		optlen += circuit_len+2;

	if (remote_len)
		optlen += remote_len+2;

	max = ((u_int8_t *)packet) + len;

	/* Commence processing after the cookie. */
	sp = op = &packet->options[0];

	while ( *op != DHCP_END && op < max )
	{
		sp = op;
		op = op + op[1] + 2;
	}

	if ( op >= max )
	{
		/*error info here*/
		return 0;
	}

	sp = op;

	*sp++ = DHCP_OPTION_AGENT_OPTIONS;
	*sp++ = optlen;

	/* Copy in the circuit id... */
	if (circuit_len)
	{
		*sp++ = RAI_CIRCUIT_ID;
		*sp++ = circuit_len;
		strncpy(sp, (const char*)circuit_id, circuit_len);
		sp += circuit_len;
	}

	/* Copy in remote ID... */
	if (remote_len)
	{
		*sp++ = RAI_REMOTE_ID;
		*sp++ = remote_len;
		strncpy(sp, (const char*)remote_id, remote_len);
		sp += remote_len;
	}

	*sp++ = DHCP_END;
	*sp++ = DHCP_PADDING;
	/* Recalculate total packet length. */
	len = sp -((u_int8_t *)packet);

	return len;
}

void ebt_dhcp_add_option82(const struct ebt_dhcp_info *info, struct sk_buff *skb)
{
	int temp=0;
	int newlen=0;
	char macString[18]={0};
	char remote_id_string[256]={0};
			
	/* construct the remote id string based on skb source mac and/or supplied string */
	if (info->dhcp_82_remote_id_mac)
		snprintf(macString,sizeof(macString),MACSTR,MAC2STR(eth_hdr(skb)->h_source));
	snprintf(remote_id_string,sizeof(remote_id_string),"%s%s",macString,info->dhcp_82_remote_id_string);

	/* check circuit id len plus 2 bytes for circuit id and length */
	if ((temp = strlen(info->dhcp_82_circuit_id)))
		newlen = temp + 2;

	if ((temp = strlen(remote_id_string)))
		newlen += temp + 2;

	if (newlen)
		newlen += 2;

	/* Expand skb if it is not big enough to hold the option content.
	   If user sets strings that goes over 256-4 bytes combined for the option , then DHCP packet
	   will be malformed, so don't go over this limit.
	 */
	if (skb_tailroom(skb) < newlen && pskb_expand_head(skb, 0, newlen - skb_tailroom(skb), GFP_ATOMIC))
		return;
	else
	{
		struct iphdr *pip = NULL;

		if ( vlan_eth_hdr(skb)->h_vlan_proto == ETH_P_IP )
		{
			pip = ip_hdr(skb);
		}
		else if ( vlan_eth_hdr(skb)->h_vlan_proto == ETH_P_8021Q )
		{
			if ( vlan_eth_hdr(skb)->h_vlan_encapsulated_proto == ETH_P_IP )
			{
				pip = (struct iphdr *)(skb_network_header(skb) + sizeof(struct vlan_hdr));
			}
		}

		if ((pip) && (pip->protocol == 17))
		{
			struct udphdr  uhead;
			int iphead_len = 0;
			iphead_len = pip->ihl << 2;
			memcpy(&uhead, (unsigned char *)pip + iphead_len, sizeof(uhead));
			if((ntohs(uhead.source) == 68)&&(ntohs(uhead.dest) == 67))
			{
				struct udphdr *udph = NULL;
				struct dhcpMessage *packet = NULL;
				int udplen = 0;

				udph = (struct udphdr *) ((unsigned char*)pip + (pip->ihl << 2) );
				udplen = ntohs(udph->len);
				packet = ( struct dhcpMessage * )((unsigned char*)udph+sizeof(struct udphdr));

				newlen = ebt_dhcp_construct_option82(info->dhcp_82_circuit_id,remote_id_string,packet,udplen);

				if ( newlen != 0 )
				{
					newlen += sizeof(struct udphdr);
					skb_put(skb, (newlen - udplen));

					udph->len = htons( newlen );
					pip->tot_len = htons( newlen + (pip->ihl << 2));

					/*udp checksum here*/
					udph->check = 0;
					udph->check = csum_tcpudp_magic(pip->saddr, pip->daddr,	newlen, IPPROTO_UDP,csum_partial(udph, newlen, 0));

					/*ip checksum here*/
					pip->check = 0;
					pip->check = ip_fast_csum((unsigned char *)pip, pip->ihl);
				}
			}
		}
	}
}

static unsigned int
ebt_dhcp_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct ebt_dhcp_info *info = par->targinfo;

	if (!skb_make_writable(skb, 0))
		return EBT_DROP;

	if (info->dhcp_82_remote_id_mac || info->dhcp_82_circuit_id[0]!='\0' || info->dhcp_82_remote_id_string[0]!='\0')
	{
		ebt_dhcp_add_option82(info, skb);
	}
	return info->target;
}

static int ebt_dhcp_tg_check(const struct xt_tgchk_param *par)
{
	const struct ebt_dhcp_info *info = par->targinfo;

	if (BASE_CHAIN && info->target == EBT_RETURN)
		return -EINVAL;

	if (INVALID_TARGET)
		return -EINVAL;

	return 0;
}

static struct xt_target ebt_dhcp_tg_reg __read_mostly = {
	.name		= EBT_DHCP_TARGET,
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.target		= ebt_dhcp_tg,
	.checkentry	= ebt_dhcp_tg_check,
	.targetsize	= XT_ALIGN(sizeof(struct ebt_dhcp_info)),
	.me		= THIS_MODULE,
};

static int __init ebt_dhcp_init(void)
{
	return xt_register_target(&ebt_dhcp_tg_reg);
}

static void __exit ebt_dhcp_fini(void)
{
	xt_unregister_target(&ebt_dhcp_tg_reg);
}

module_init(ebt_dhcp_init);
module_exit(ebt_dhcp_fini);
MODULE_DESCRIPTION("Ebtables: DHCP option modification");
MODULE_LICENSE("GPL");
