/* ebt_pcredirect
 *
 * Authors:
 *
 * July, 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/ebtables_u.h"
#include "../include/linux/netfilter_bridge/ebt_pcredirect.h"

#define PCREDIRECT_TARGET '1'
#define PCREDIRECT_IP     '2'
static struct option opts[] =
{
	{ "pcredirect-target", required_argument, 0, PCREDIRECT_TARGET },
        { "pcredirect-ip", required_argument, 0, PCREDIRECT_IP },
	{ 0 }
};

static void print_help()
{
	printf(
	"pcredirect option:\n"
	" --pcredirect-target target   : ACCEPT, DROP, RETURN or CONTINUE\n"
        " --pcredirect-ip ip           : 1.0.0.1/::ffff:1.0.0.1\n");
}

static void init(struct ebt_entry_target *target)
{
	struct ebt_pcredirect_info *pcredirectinfo =
	   (struct ebt_pcredirect_info *)target->data;

	pcredirectinfo->target = EBT_ACCEPT;
	return;
}

#define OPT_PCREDIRECT_TARGET  0x01
#define OPT_PCREDIRECT_IP      0x02
static int parse(int c, char **argv, int argc,
   const struct ebt_u_entry *entry, unsigned int *flags,
   struct ebt_entry_target **target)
{
    char *p=NULL;
	struct ebt_pcredirect_info *pcredirectinfo =
	   (struct ebt_pcredirect_info *)(*target)->data;

	switch (c) {
	case PCREDIRECT_TARGET:
		ebt_check_option2(flags, OPT_PCREDIRECT_TARGET);
		if (FILL_TARGET(optarg, pcredirectinfo->target))
			ebt_print_error2("Illegal --pcredirect-target target");
		break;
    case PCREDIRECT_IP:
        ebt_check_option2(flags, OPT_PCREDIRECT_IP);
        p=strchr(optarg,'/');
        if(p)
        {
            *p='\0'; 
            if(!inet_aton(optarg,&pcredirectinfo->redirect_ip))
            {
                ebt_print_error2("Illegal --pcredirect-ip ip");
            }else
            {
                if(!inet_pton(AF_INET6,p+1,&pcredirectinfo->redirect_ip6))
                {
                    ebt_print_error2("Illegal --pcredirect-ip ip");
                }
            }
        }else
        {
             ebt_print_error2("Illegal --pcredirect-ip ip");
        }
        break;
	default:
		return 0;
	}
	return 1;
}

static void final_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_target *target, const char *name,
   unsigned int hookmask, unsigned int time)
{
	struct ebt_pcredirect_info *pcredirectinfo =
	   (struct ebt_pcredirect_info *)target->data;

	if (BASE_CHAIN && pcredirectinfo->target == EBT_RETURN) {
		ebt_print_error("--pcredirect-target RETURN not allowed on base chain");
		return;
	}
}

static void print(const struct ebt_u_entry *entry,
   const struct ebt_entry_target *target)
{
	struct ebt_pcredirect_info *pcredirectinfo =
	   (struct ebt_pcredirect_info *)target->data;
    char ip6_address[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6,&(pcredirectinfo->redirect_ip6),ip6_address,INET6_ADDRSTRLEN);
    printf(" --pcredirect-ip %s/%s", inet_ntoa(pcredirectinfo->redirect_ip), ip6_address);
	if (pcredirectinfo->target == EBT_ACCEPT)
		return;
	printf(" --pcredirect-target %s", TARGET_NAME(pcredirectinfo->target));
        
}

static int compare(const struct ebt_entry_target *t1,
   const struct ebt_entry_target *t2)
{
	struct ebt_pcredirect_info *pcredirectinfo1 =
	   (struct ebt_pcredirect_info *)t1->data;
	struct ebt_pcredirect_info *pcredirectinfo2 =
	   (struct ebt_pcredirect_info *)t2->data;

	return pcredirectinfo1->target == pcredirectinfo2->target;
}

static struct ebt_u_target pcredirect_target =
{
	.name		= "pcredirect",
	.size		= sizeof(struct ebt_pcredirect_info),
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
	ebt_register_target(&pcredirect_target);
}
