/* ebt_domainname
 * 
 * Authors:
 *
 * June, 2016
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include "../include/ebtables_u.h"
#include "../include/ethernetdb.h"
#include "../include/linux/netfilter_bridge/ebt_domainname.h"
#define NAME_DOMAINNAME    "domainname"

#define DOMAINNAME    '1'
#define SAVEWHITELIST       '2'
#define MATCHWHITELIST      '3'

static struct option opts[] = {
	{"domainname", required_argument, NULL, DOMAINNAME},
        {"saveWhiteList",  no_argument, NULL, SAVEWHITELIST},
        {"matchWhiteList",  no_argument, NULL, MATCHWHITELIST},
	{ 0 }
};

/*
 * option inverse flags definition 
 */

#define OPT_DOMAINNAME     0x01
#define OPT_DOMAINNAME_FLAGS	(OPT_DOMAINNAME)

struct ethertypeent *ethent;

static void print_help()
{
    printf(
"domainname options:\n"
"--domainname  domainname      : dns query domainname frame identifier, 1-255 (char),www.xxxx.com\n"
"--saveWhiteList: cache 10 newest whitelist recorders\n"
"--matchWhiteList: match with those 10 newest whitelist recorders.\n");
	
}

static void init(struct ebt_entry_match *match)
{
	struct ebt_domainname_info *domainnameinfo = (struct ebt_domainname_info *) match->data;
        memset(domainnameinfo->domainname,0,DOMAINNAME_SIZE);
        domainnameinfo->matchWhiteList=false;
        domainnameinfo->saveWhiteList=false;
}


static int parse(int c, char **argv, int argc, const struct ebt_u_entry *entry,
   unsigned int *flags, struct ebt_entry_match **match)
{

	struct ebt_domainname_info *domainnameinfo = (struct ebt_domainname_info *) (*match)->data;
	switch (c) {
	case DOMAINNAME:
                if (*flags & EBT_DOMAINNAME)
                      ebt_print_error("Can't specify --domainname twice");
		ebt_check_option2(flags, OPT_DOMAINNAME);
		if (strlen(optarg) >= DOMAINNAME_SIZE)
			ebt_print_error2("--domain string value too long");
		strncpy(domainnameinfo->domainname, optarg, DOMAINNAME_SIZE);
		*flags |= EBT_DOMAINNAME;
		break;
        case SAVEWHITELIST:
                if (*flags & EBT_SAVEWHITELIST)
                      ebt_print_error2("Can't specify --saveWhiteList twice");
                if (*flags & EBT_MATCHWHITELIST)
                      ebt_print_error2("Can't set --saveWhiteList as matchWhiteList already set");
                if(!(*flags & EBT_DOMAINNAME))
                      ebt_print_error2("Can't set --saveWhiteList as --domainname not set");
                domainnameinfo->saveWhiteList=true;
                *flags |= EBT_SAVEWHITELIST;
                break;
        case MATCHWHITELIST:
                if (*flags & EBT_MATCHWHITELIST)
                      ebt_print_error2("Can't specify --matchWhiteList twice");
                if (*flags & EBT_SAVEWHITELIST)
                      ebt_print_error2("Can't set --matchWhiteList as --saveWhiteList already set");
                domainnameinfo->matchWhiteList=true;
                *flags |= EBT_MATCHWHITELIST;
                break;
	default:
		return 0;

	}
	return 1;
}

static void final_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match,
   const char *name, unsigned int hookmask, unsigned int time)
{

}

static void print(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match)
{

	struct ebt_domainname_info *domainnameinfo = (struct ebt_domainname_info *) match->data;
        if(domainnameinfo->domainname && strlen(domainnameinfo->domainname) > 0) printf("--domainname %s ", domainnameinfo->domainname);
        if(domainnameinfo->saveWhiteList) printf("--saveWhiteList ");
        if(domainnameinfo->matchWhiteList) printf("--matchWhiteList ");

}

static int compare(const struct ebt_entry_match *domainname1,
   const struct ebt_entry_match *domainname2)
{

        int i;
	struct ebt_domainname_info *domainnameinfo1 = (struct ebt_domainname_info *) domainname1->data;
	struct ebt_domainname_info *domainnameinfo2 = (struct ebt_domainname_info *) domainname2->data;

        for(i=0;i<DOMAINNAME_SIZE;i++)
        { 
            if(domainnameinfo1->domainname[i] != domainnameinfo2->domainname[i])
            return 0;
        } 

	return 1;
}

static struct ebt_u_match domainname_match = {
	.name		= "domainname",
	.size		= sizeof(struct ebt_domainname_info),
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
	ebt_register_match(&domainname_match);
}
