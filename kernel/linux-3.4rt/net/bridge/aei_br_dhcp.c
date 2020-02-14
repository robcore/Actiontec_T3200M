/*
 *	Handle incoming dhcp frames
 *	Linux ethernet bridge
 *
 *	refer to gpl code udhcp-0.9.8
 *
 */

#if defined(GPL_CODE_BRIDGE_STB_QOS)

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/times.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/jhash.h>
#include <asm/atomic.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/netlink.h>
#include "aei_br_dhcp.h"
#include "../../../../shared/opensource/include/bcm963xx/bcmnetlink.h"

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

struct udp_dhcp_pkt {
        struct udphdr udp;
        struct dhcpMessage data;
};

#define OPT_CODE 0
#define OPT_LEN 1

#define DHCP_PADDING            0x00
#define DHCP_OPTION_OVER        0x34
#define DHCP_END                0xFF

#define OPTION_FIELD            0
#define FILE_FIELD              1
#define SNAME_FIELD             2



unsigned char *get_dhcp_option(struct dhcpMessage *packet, int code)
{
    int i, length;
    unsigned char *optionptr;
    int over = 0, done = 0, curr = OPTION_FIELD;

    optionptr = packet->options;
    i = 0;
    length = 308;
    while (!done) {
        if (i >= length) {
            return NULL;
        }
        if (optionptr[i + OPT_CODE] == code) {
            if (i + 1 + optionptr[i + OPT_LEN] >= length) {
                return NULL;
            }
            return optionptr + i + 2;
        }
        switch (optionptr[i + OPT_CODE]) {
        case DHCP_PADDING:
            i++;
            break;
        case DHCP_OPTION_OVER:
            if (i + 1 + optionptr[i + OPT_LEN] >= length) {
                return NULL;
            }
            over = optionptr[i + 3];
            i += optionptr[OPT_LEN] + 2;
            break;
        case DHCP_END:
            if (curr == OPTION_FIELD && over & FILE_FIELD) {
                optionptr = packet->file;
                i = 0;
                length = 128;
                curr = FILE_FIELD;
            } else if (curr == FILE_FIELD && over & SNAME_FIELD) {
                optionptr = packet->sname;
                i = 0;
                length = 64;
                curr = SNAME_FIELD;
            } else
                done = 1;
            break;
        default:
            i += optionptr[OPT_LEN + i] + 2;
        }
    }
    return NULL;
}

int check_dhcp_option(struct dhcpMessage *packet, char *tmp_vendorid)
{
        unsigned char * state;
        char * vendorid;
        int len;

        state = get_dhcp_option(packet, 53);  /*option 53*/
        if(!state)
        {
                printk("no option 53\n");
                return 0;
        }

        vendorid = (char *)get_dhcp_option(packet, 60);  /*option 60*/

        if(!vendorid)
        {
                //printk("state[%d]  no option 60\n", state[0]);
                return 0;
        }

        len = *(vendorid - 1);

        if ( len > 31 ) 
        {
           /*for option60 string len larger than 31, only get the first 31 bytes*/
            len = 31;
        }
        memcpy(tmp_vendorid, vendorid, len);
      
        return 1;
}

void aei_handle_dhcp_packet(const struct sk_buff *pskb, struct udphdr *pudp)
{
       struct udp_dhcp_pkt packet;
       char tmp_vendorid[32];
       int cpylen = 0;
       int udlen = 0;

       if (pskb->dev && (pskb->dev->priv_flags & IFF_WANDEV)
#if defined(GPL_CODE_WAN_CONVERT_LAN)
		&& !(pskb->dev->priv_flags & IFF_AEI_WAN_TO_LAN)
#endif
	)     //only process LAN Side dhcp packet.
           return;

       memset(tmp_vendorid, 0, sizeof(tmp_vendorid));
       udlen = ntohs(pudp->len);
       cpylen = sizeof(packet) < udlen?sizeof(packet):udlen;
       memcpy(&packet, pudp, cpylen);

       if (check_dhcp_option(&(packet.data), tmp_vendorid))
       {
            char tmp_string[64];
            unsigned char * mac;
          
            memset(tmp_string, 0, sizeof(tmp_string));
            mac = eth_hdr(pskb)->h_source;
            sprintf(tmp_string, "%02X:%02X:%02X:%02X:%02X:%02X;", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            strcat(tmp_string, tmp_vendorid);
          
            // send source mac and option60 to ssk.
            kerSysSendtoMonitorTask(MSG_NETLINK_AEI_BR_DHCP_OPT60, tmp_string, sizeof(tmp_string));
       }
}
#endif
