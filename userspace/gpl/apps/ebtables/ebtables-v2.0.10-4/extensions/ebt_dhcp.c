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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ether.h>
#include <getopt.h>
#include "../include/ebtables_u.h"
#include "../include/linux/netfilter_bridge/ebt_dhcp.h"

//static int to_source_supplied, to_dest_supplied;

#define DHCP_TARGET '1'
#define DHCP_82_CIRCUIT_ID '2'
#define DHCP_82_REMOTE_ID_MAC '4'
#define DHCP_82_REMOTE_ID_STRING '8'

static struct option opts[] =
{
	{ "dhcp-target"         , required_argument, 0, DHCP_TARGET},
	{ "82-circuit-id"       , required_argument, 0, DHCP_82_CIRCUIT_ID },
	{ "82-remote-id-mac"    , no_argument      , 0, DHCP_82_REMOTE_ID_MAC },
	{ "82-remote-id-string" , required_argument, 0, DHCP_82_REMOTE_ID_STRING },
	{ 0 }
};

static void print_help()
{
	printf(
	"dhcp options:\n"
	" --82-circuit-id       : insert following string in option 82 circuit ID suboption\n"
	" --82-remote-id-mac    : insert source MAC of client DHCP packet in option 82 remote ID suboption\n"
	" --82-remote-id-string : insert following string in option 82 remote ID suboption\n"
	" --dhcp-target    : ACCEPT, DROP, RETURN or CONTINUE\n");
}

static void init(struct ebt_entry_target *target)
{
	struct ebt_dhcp_info *dhcpinfo =
		(struct ebt_dhcp_info *)target->data;

	dhcpinfo->target = EBT_CONTINUE;
	dhcpinfo->dhcp_82_remote_id_mac = 0;
}



#define OPT_DHCP_TARGET              0x01
#define OPT_DHCP_82_CIRCUIT_ID       0x02
#define OPT_DHCP_82_REMOTE_ID_MAC    0x04
#define OPT_DHCP_82_REMOTE_ID_STRING 0x08

static int parse(int c, char **argv, int argc,
   const struct ebt_u_entry *entry, unsigned int *flags,
   struct ebt_entry_target **target)
{
	struct ebt_dhcp_info *dhcpinfo = (struct ebt_dhcp_info *)(*target)->data;

	switch (c) {
	case DHCP_82_CIRCUIT_ID:
		ebt_check_option2(flags, OPT_DHCP_82_CIRCUIT_ID);
		if (strlen(optarg) >= sizeof(dhcpinfo->dhcp_82_circuit_id))
			ebt_print_error2("--82-circuit-id value too long");
		strncpy(dhcpinfo->dhcp_82_circuit_id, optarg,sizeof(dhcpinfo->dhcp_82_circuit_id));
		break;
	case DHCP_82_REMOTE_ID_MAC:
		ebt_check_option2(flags, OPT_DHCP_82_REMOTE_ID_MAC);
		dhcpinfo->dhcp_82_remote_id_mac=1;		
		break;
	case DHCP_82_REMOTE_ID_STRING:
		ebt_check_option2(flags, OPT_DHCP_82_REMOTE_ID_STRING);
		if (strlen(optarg) >= sizeof(dhcpinfo->dhcp_82_remote_id_string))
			ebt_print_error2("--82-remote-id_string value too long");
		strncpy(dhcpinfo->dhcp_82_remote_id_string, optarg,sizeof(dhcpinfo->dhcp_82_remote_id_string));
		break;
	case DHCP_TARGET:
		ebt_check_option2(flags, OPT_DHCP_TARGET);
		if (FILL_TARGET(optarg, dhcpinfo->target))
			ebt_print_error2("Illegal --dhcp-target target");
		break;
	default:
		return 0;
	}
	return 1;
}


static void
final_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_target *target, const char *name,
   unsigned int hookmask, unsigned int time)
{
	struct ebt_dhcp_info *dhcpinfo = (struct ebt_dhcp_info *)target->data;

	if (BASE_CHAIN && dhcpinfo->target == EBT_RETURN)
		ebt_print_error("--dhcp-target RETURN not allowed on base chain");
}



static void print(const struct ebt_u_entry *entry,
   const struct ebt_entry_target *target)
{
	struct ebt_dhcp_info *dhcpinfo = (struct ebt_dhcp_info *)target->data;

	if (dhcpinfo->dhcp_82_circuit_id[0]!='\0')
		printf("--82-circuit-id %s ",dhcpinfo->dhcp_82_circuit_id);

	if (dhcpinfo->dhcp_82_remote_id_mac)
		printf("--82-remote-id-mac ");

	if (dhcpinfo->dhcp_82_remote_id_string[0]!='\0')
		printf("--82-remote-id_string %s ",dhcpinfo->dhcp_82_remote_id_string);

	printf("--dhcp-target %s", TARGET_NAME(dhcpinfo->target));
}



static int 
compare(const struct ebt_entry_target *t1,
  	 const struct ebt_entry_target *t2)
{
	struct ebt_dhcp_info *dhcpinfo1 =
	   (struct ebt_dhcp_info *)t1->data;
	struct ebt_dhcp_info *dhcpinfo2 =
	   (struct ebt_dhcp_info *)t2->data;

	return (dhcpinfo1->target == dhcpinfo2->target &&
	   dhcpinfo1->dhcp_82_remote_id_mac == dhcpinfo2->dhcp_82_remote_id_mac &&
	   !strncmp(dhcpinfo1->dhcp_82_circuit_id,dhcpinfo2->dhcp_82_circuit_id,sizeof(dhcpinfo1->dhcp_82_circuit_id)) &&
	   !strncmp(dhcpinfo1->dhcp_82_remote_id_string,dhcpinfo2->dhcp_82_remote_id_string,sizeof(dhcpinfo1->dhcp_82_remote_id_string))
	);
}


static struct ebt_u_target dhcp_target =
{
	.name		= EBT_DHCP_TARGET,
	.size		= sizeof(struct ebt_dhcp_info),
	.help		= print_help,
	.init		= init,
	.parse		= parse,
	.final_check	= final_check,
	.print		= print,
	.compare	= compare,
	.extra_ops	= opts,
};

void _init(void)
{
	ebt_register_target(&dhcp_target);
}
