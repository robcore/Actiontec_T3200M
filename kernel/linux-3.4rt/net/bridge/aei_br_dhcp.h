/*
 *  Handle incoming dhcp frames
 *  Linux ethernet bridge
 *
 *  support for STB QoS of Bridge mode.
 *
 */

#ifndef _AEI_BR_DHCP_H
#define _AEI_BR_DHCP_H

#if defined(GPL_CODE_BRIDGE_STB_QOS)

#include <linux/udp.h>

#define DHCP_SVR_PORT	67

void aei_handle_dhcp_packet(const struct sk_buff *pskb, struct udphdr *pudp);

#endif

#endif
