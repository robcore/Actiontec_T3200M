/* clientpacket.c
 *
 * Packet generation and dispatching functions for the DHCP client.
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <string.h>
#include <sys/socket.h>
#include <features.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "dhcpd.h"
#include "packet.h"
#include "options.h"
#include "dhcpc.h"
#include "debug.h"
#ifdef GPL_CODE
#include <sys/stat.h>
char option55_list[OPTION55_MAX_LEN] = {0};
#endif

#if defined(GPL_CODE_ADVANCED_DMZ)
extern char server_hwaddr[6];
#endif
/* Create a random xid */
unsigned long random_xid(void)
{
	static int initialized;
	if (!initialized) {
		srand(time(0));
		initialized++;
	}
	return rand();
}

#if defined(GPL_CODE)
/* get an integer between -1000X1000 and 1000X1000 */
int get_randomize_usec(void)
{
    int usec = 0;
    srand(time(0));
    usec =  (int)(rand() % 2000 - 1000);
    return (usec * 1000);
}

static int check_duplicated_option_number(char option_list[], int len, int option)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (option_list[i] == option)
            return 1;
    }

    return 0;
}

static int AEI_parse_option55(char *mdm_op)
{
    char tmp_str[BUFLEN_1024] = {0};
    char *p = NULL;
    char *option_value = NULL;
    int option_num = 0;
    int len = 0;

    if (mdm_op == NULL)
        return 0;

    strncpy(tmp_str, mdm_op, sizeof(tmp_str));
    p = tmp_str;

    while ((option_value = strsep(&p, ",")) != NULL)
    {
        option_num = atoi(option_value);
        if ((option_num > 0) && check_duplicated_option_number(option55_list, len, option_num))
            continue;
        else
        {
            if (len < OPTION55_MAX_LEN-1)
            {
                option55_list[len] = option_num;
                len++;
            }
            else 
                break;
        }
    }

    return len;
}

static int get_option55_values(void)
{
    int result = 0;
    FILE *fp_option = NULL;
    struct stat buf;
    char *option55 = NULL;
    int count = 0;
    char filename[BUFLEN_128] = {0};

    if (strstr(vendor_class_id, "IPTV"))
        snprintf(filename, sizeof(filename), "/var/dhcpc_op55_IPTV");
    else if (strstr(vendor_class_id, "HSI"))
        snprintf(filename, sizeof(filename), "/var/dhcpc_op55_HSI");

    result = stat(filename, &buf);

    memset(option55_list, 0 ,sizeof(option55_list));

    if ((result != 0) || (buf.st_size == 0))
    {
        //LOG(LOG_ERR, "There is no option value in file %s", filename);
        return 0;
    }
    option55 = (char *)calloc(buf.st_size + 1, sizeof(char));

    if (option55 == NULL)
    {
        //LOG(LOG_ERR, "calloc failed");
        return 0;
    }

    if ((fp_option = fopen(filename, "r")) != NULL) {
        fread(option55, 1, buf.st_size, fp_option);
        fclose(fp_option);
        count = AEI_parse_option55(option55);
    } else {
        //LOG(LOG_ERR, "Can not open %s", filename);
        count = 0;
    }

    free(option55);
    option55 = NULL;
    return count;
}
#endif

/* initialize a packet with the proper defaults */
static void init_packet(struct dhcpMessage *packet, char type)
{
	//brcm
    char VSI[256];
    char client_id[256];	
	struct vendor  {
		char vendor, length;
#ifdef GPL_CODE
        char str[256];
#else
		char str[sizeof("uDHCP "VERSION)];
#endif
	} vendor_id = { DHCP_VENDOR,  sizeof("uDHCP "VERSION) - 1, "uDHCP "VERSION};
	
	struct user_class  {
		char user_class, length;
		char uc_length;
		char str[128];
	} user_cl_id = { DHCP_USER_CLASS_ID, 0, 0, ""};
	
	memset(packet, 0, sizeof(struct dhcpMessage));
	
	packet->op = BOOTREQUEST;
	packet->htype = ETH_10MB;
	packet->hlen = ETH_10MB_LEN;
	packet->cookie = htonl(DHCP_MAGIC);
	packet->options[0] = DHCP_END;
	memcpy(packet->chaddr, client_config.arp, 6);
	add_simple_option(packet->options, DHCP_MESSAGE_TYPE, type);
	if (client_config.clientid) add_option_string(packet->options, (unsigned char *)(client_config.clientid));
	if (client_config.hostname) add_option_string(packet->options, (unsigned char *)(client_config.hostname));
	// brcm
	if (en_vendor_class_id) { 
	   if (strlen(vendor_class_id)) {
	       vendor_id.length = strlen(vendor_class_id);
#ifdef GPL_CODE
            snprintf(vendor_id.str, sizeof(vendor_id.str)-1, "%s", vendor_class_id);
#else
	       sprintf(vendor_id.str, "%s", vendor_class_id);
#endif
	       add_option_string(packet->options, (unsigned char *)&vendor_id);
	   } else {
	       add_option_string(packet->options, (unsigned char *)&vendor_id);
	   }
	}
	if (en_user_class_id) { 
	   if (strlen(user_class_id)) {
	       user_cl_id.length = strlen(user_class_id) + 1;
	       user_cl_id.uc_length = strlen(user_class_id);
	       sprintf(user_cl_id.str, "%s", user_class_id);
	       add_option_string(packet->options, (unsigned char *)&user_cl_id);
	   }
	}

	if (en_125){
#ifdef GPL_CODE
       if ( CreateOption125(VENDOR_IDENTIFYING_FOR_GATEWAY, oui_125, sn_125, prod_125, mn_125, VSI) != -1 ) {
#else
       if ( CreateOption125(VENDOR_IDENTIFYING_FOR_DEVICE, oui_125, sn_125, prod_125, VSI) != -1 ) {
#endif
           add_option_string(packet->options, (unsigned char *)VSI);
	   }
	}
    if ( en_client_id == 1 ){
	   if (strlen(iaid) && strlen(duid)) {
	       if (CreateClntId(iaid, duid, client_id) != -1)
	          add_option_string(packet->options, (unsigned char *)client_id);
	   }
	}
}


/* Add a paramater request list for stubborn DHCP servers */
static void add_requests(struct dhcpMessage *packet)
{
	if (ipv6rd_opt)
	{
		char request_list[] = {DHCP_PARAM_REQ, 0, PARM_REQUESTS, DHCP_6RD_OPT};
		request_list[OPT_LEN] = sizeof(request_list) - 2;
		add_option_string(packet->options, (unsigned char *)request_list);
	}
	else
	{
#ifdef GPL_CODE
    char request_list[OPTION55_MAX_LEN] = {DHCP_PARAM_REQ, 0, PARM_REQUESTS
#if defined(GPL_CODE_DHCP_WAN_OPTION121)
            ,DHCP_CLASSLESS_ROUTE
#endif
            ,DHCP_VENDOR_ID
    };

#if defined(GPL_CODE)
    int count = 0;
    count = get_option55_values();
    if (count > 0) 
    {
        int current_num = 0;
        int j = 0;

        current_num = cmsUtl_strlen(request_list + 2) + 2;
        for (j = 0; j < count; j++)
        {
            if (option55_list[j] > 0)
            {
                if (check_duplicated_option_number(request_list, current_num, option55_list[j]))
                {
                    continue;
                }
                else
                {
                    request_list[current_num] = option55_list[j];
                    current_num++;
                }
            }
        }
    }
    request_list[OPT_LEN] = cmsUtl_strlen(request_list + 2);
#endif
#else
#if defined(GPL_CODE_DHCP_WAN_OPTION121)
	char request_list[] = { DHCP_PARAM_REQ, 0,DHCP_CLASSLESS_ROUTE,PARM_REQUESTS
        };
#else
    char request_list[] = { DHCP_PARAM_REQ, 0, PARM_REQUESTS
    };
#endif
		request_list[OPT_LEN] = sizeof(request_list) - 2;
#endif
		add_option_string(packet->options, (unsigned char *)request_list);
	}
}


/* Broadcast a DHCP discover packet to the network, with an optionally requested IP */
int send_discover(unsigned long xid, unsigned long requested)
{
	struct dhcpMessage packet;

	init_packet(&packet, DHCPDISCOVER);
	packet.xid = xid;
	if (requested)
		add_simple_option(packet.options, DHCP_REQUESTED_IP, ntohl(requested));

	add_requests(&packet);
	// brcm
	// LOG(LOG_DEBUG, "Sending discover...");
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST, 
				SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}


/* Broadcasts a DHCP request message */
int send_selecting(unsigned long xid, unsigned long server, unsigned long requested)
{
	struct dhcpMessage packet;
	// brcm
	//struct in_addr addr;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;

	/* expects host order */
	add_simple_option(packet.options, DHCP_REQUESTED_IP, ntohl(requested));

	/* expects host order */
	add_simple_option(packet.options, DHCP_SERVER_ID, ntohl(server));
	
	add_requests(&packet);
	// brcm
	//addr.s_addr = requested;
	// brcm
	//LOG(LOG_DEBUG, "Sending select for %s...", inet_ntoa(addr));
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST, 
				SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}


/* Unicasts or broadcasts a DHCP renew message */
int send_renew(unsigned long xid, unsigned long server, unsigned long ciaddr)
{
	struct dhcpMessage packet;
	int ret = 0;
#if defined(GPL_CODE) || defined(GPL_CODE)
    FILE *fp;
#endif

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;
	packet.ciaddr = ciaddr;

#ifdef GPL_CODE
	add_requests(&packet);
#endif

	LOG(LOG_DEBUG, "Sending renew...");
	if (server) 
        {
		ret = kernel_packet(&packet, ciaddr, CLIENT_PORT, server, SERVER_PORT);
#if defined(GPL_CODE_ADVANCED_DMZ)
		if (ret < 0) {  /* If error occured try to use the raw mode if MAC is not invalid */
			LOG(LOG_ERR, "ret=%d Sending renew... raw \n",ret);
			LOG(LOG_ERR, "raw server address %02x:%02x:%02x:%02x:%02x:%02x",
			 server_hwaddr[0], server_hwaddr[1],server_hwaddr[2],
			server_hwaddr[3], server_hwaddr[4], server_hwaddr[5]);
			 if(!memcmp(server_hwaddr,MAC_INVALID_ADDR,6))
			 memcpy(server_hwaddr, MAC_BCAST_ADDR, 6);

			 ret = raw_packet(&packet, ciaddr, CLIENT_PORT, server,
                             SERVER_PORT, server_hwaddr, client_config.ifindex);
			if(ret >= 0)
			   ret = 0; // fake value, just for return
		 }
#endif

	 } 
         else ret = raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
				SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
#if defined(GPL_CODE) || defined(GPL_CODE)
	 if ((fp = fopen("/var/renew_completed", "w")))
	{
		fwrite("1", sizeof(char), sizeof(char), fp);
		fclose(fp);
	}
#endif
	return ret;
}	


/* Unicasts a DHCP release message */
int send_release(unsigned long server, unsigned long ciaddr)
{
	struct dhcpMessage packet;

	init_packet(&packet, DHCPRELEASE);
	packet.xid = random_xid();
	packet.ciaddr = ciaddr;
	
	/* expects host order */
	add_simple_option(packet.options, DHCP_REQUESTED_IP, ntohl(ciaddr));
	add_simple_option(packet.options, DHCP_SERVER_ID, ntohl(server));

	LOG(LOG_DEBUG, "Sending release...");
	return kernel_packet(&packet, ciaddr, CLIENT_PORT, server, SERVER_PORT);
#if defined(GPL_CODE) || defined(GPL_CODE)
    system("echo 1 > /var/release_completed");
#endif
}


int get_raw_packet(struct dhcpMessage *payload, int fd)
{
	int bytes;
	struct udp_dhcp_packet packet;
	u_int32_t source, dest;
	u_int16_t check;

	memset(&packet, 0, sizeof(struct udp_dhcp_packet));
	bytes = read(fd, &packet, sizeof(struct udp_dhcp_packet));
	if (bytes < 0) {
		DEBUG(LOG_INFO, "couldn't read on raw listening socket -- ignoring");
		usleep(500000); /* possible down interface, looping condition */
		return -1;
	}
	
	if (bytes < (int) (sizeof(struct iphdr) + sizeof(struct udphdr))) {
		DEBUG(LOG_INFO, "message too short, ignoring");
		return -1;
	}

   /*
    * The minimum ethernet frame for 1000Base-X is 416 bytes,
    * and the miminum ethernet frame for 1000Base-T is 520 bytes,
    * so these small DHCP packets may get padded.
    * Change the size checking below to just ensure byte count
    * is at least as long as the length fields in the ip and
    * udp headers.  But don't expect exact length match anymore.  --mwang
    */
    
	/* Make sure its the right packet for us, and that it passes sanity checks */
	if (packet.ip.protocol != IPPROTO_UDP || packet.ip.version != IPVERSION ||
	    packet.ip.ihl != sizeof(packet.ip) >> 2 || packet.udp.dest != htons(CLIENT_PORT) ||
	    ntohs(packet.ip.tot_len) > bytes ||
	    ntohs(packet.udp.len) > (short) (bytes - sizeof(packet.ip)) ||
#ifdef GPL_CODE
       (sizeof(struct udp_dhcp_packet) - UDHCP_OPTIONS_LEN) > (unsigned int) bytes) {  
#else
       (sizeof(struct udp_dhcp_packet) - 308) > (unsigned int) bytes) {
#endif

	    	DEBUG(LOG_INFO, "unrelated/bogus packet");
	    	return -1;
	}

	/* check IP checksum */
	check = packet.ip.check;
	packet.ip.check = 0;
	if (check != checksum(&(packet.ip), sizeof(packet.ip))) {
		DEBUG(LOG_INFO, "bad IP header checksum, ignoring");
		return -1;
	}
	
	/* verify the UDP checksum by replacing the header with a psuedo header */
	source = packet.ip.saddr;
	dest = packet.ip.daddr;
	check = packet.udp.check;
	packet.udp.check = 0;
	memset(&packet.ip, 0, sizeof(packet.ip));

	packet.ip.protocol = IPPROTO_UDP;
	packet.ip.saddr = source;
	packet.ip.daddr = dest;
	packet.ip.tot_len = packet.udp.len; /* cheat on the psuedo-header */
	// brcm
     
#if defined(GPL_CODE)
       if (check!=0 && check != checksum(&packet, bytes)) {
       DEBUG(LOG_ERR, "packet with bad UDP checksum received, ignoring");
       // cwu
         return -1;
       }
#endif
	
	memcpy(payload, &(packet.data), bytes - (sizeof(packet.ip) + sizeof(packet.udp)));
	
	if (ntohl(payload->cookie) != DHCP_MAGIC) {
		LOG(LOG_ERR, "received bogus message (bad magic) -- ignoring");
		return -1;
	}
	DEBUG(LOG_INFO, "oooooh!!! got some!");
	return bytes - (sizeof(packet.ip) + sizeof(packet.udp));
}
