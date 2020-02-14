#ifndef __LINUX_BRIDGE_EBT_DHCP_H
#define __LINUX_BRIDGE_EBT_DHCP_H

struct ebt_dhcp_info
{
	char dhcp_82_circuit_id[256];
	char dhcp_82_remote_id_string[256];
	int target; /* EBT_ACCEPT, EBT_DROP, EBT_CONTINUE or EBT_RETURN */
	uint8_t  dhcp_82_remote_id_mac;
};

#define EBT_DHCP_TARGET "dhcp"

#endif
