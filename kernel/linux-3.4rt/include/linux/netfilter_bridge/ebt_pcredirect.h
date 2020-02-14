#ifndef __LINUX_BRIDGE_EBT_DHCP_H
#define __LINUX_BRIDGE_EBT_DHCP_H
#include <linux/types.h>
#include <linux/socket.h>


struct ebt_pcredirect_info
{
	struct in_addr redirect_ip;
    struct in6_addr redirect_ip6;
	int target; /* EBT_ACCEPT, EBT_DROP, EBT_CONTINUE or EBT_RETURN */
};

#define EBT_PCREDIRECT_TARGET "pcredirect"

#endif
