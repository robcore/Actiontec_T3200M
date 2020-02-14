/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation (the "GPL").
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    
    A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
    writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    
    :>
*/

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation of the Runner CPU ring interface     */
/*                                                                            */
/******************************************************************************/

#ifndef _RDP_CPU_RING_H_
#define _RDP_CPU_RING_H_

#ifndef RDP_SIM

#if defined(__KERNEL__) || defined(_CFE_)

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include<bcm_pkt_lengths.h>

#ifdef _CFE_
#include "bl_os_wraper.h"
#endif
#include "rdpa_types.h"
#include "rdpa_cpu_basic.h"
#include "rdd.h"
#include <bcm_mm.h>
#include "rdp_cpu_ring_defs.h"

/*****************************************************************************/
/*                                                                           */
/* defines and structures                                                    */
/*                                                                           */
/*****************************************************************************/


#ifdef _CFE_

#include "lib_malloc.h"
#include "cfe_iocb.h"
#define RDP_CPU_RING_MAX_QUEUES	1
#define RDP_WLAN_MAX_QUEUES		0

#elif defined(__KERNEL__)

#include "rdpa_cpu.h"
#include "bdmf_system.h"
#include "bdmf_shell.h"
#include "bdmf_dev.h"

#define RDP_CPU_RING_MAX_QUEUES		RDPA_CPU_MAX_QUEUES
#define RDP_WLAN_MAX_QUEUES		RDPA_WLAN_MAX_QUEUES

extern const bdmf_attr_enum_table_t rdpa_cpu_reason_enum_table;

#endif

typedef struct
{
   uint8_t* data_ptr;
   uint16_t packet_size;
   uint16_t flow_id;
   uint16_t reason;
   uint16_t src_bridge_port;
   uint16_t dst_ssid;
   uint16_t wl_metadata; 
   uint16_t ptp_index;
   uint16_t free_index;
   uint8_t  is_rx_offload;
   uint8_t  is_ipsec_upstream;
}
CPU_RX_PARAMS;

#define MAX_BUFS_IN_CACHE 32 
typedef struct
{
	uint32_t	   ring_id;
	uint32_t	   admin_status;
	uint32_t	   num_of_entries;
	uint32_t	   size_of_entry;
	uint32_t	   packet_size;
	CPU_RX_DESCRIPTOR* head;
	CPU_RX_DESCRIPTOR* base;
	CPU_RX_DESCRIPTOR* end;
	uint32_t           buff_cache_cnt;
	uint32_t*          buff_cache;
}
RING_DESCTIPTOR;

/*array of possible rings private data*/
#define D_NUM_OF_RING_DESCRIPTORS (RDP_CPU_RING_MAX_QUEUES + RDP_WLAN_MAX_QUEUES)


int rdp_cpu_ring_create_ring(uint32_t ringId,uint32_t entries, uint32_t*  ring_head);

int rdp_cpu_ring_delete_ring(uint32_t ringId);

int rdp_cpu_ring_read_packet_copy(uint32_t ringId, CPU_RX_PARAMS* rxParams);

int rdp_cpu_ring_get_queue_size(uint32_t ringId);

int rdp_cpu_ring_get_queued(uint32_t ringId);

int rdp_cpu_ring_flush(uint32_t ringId);

int rdp_cpu_ring_not_empty(uint32_t ringId);

#endif /* if defined(__KERNEL__) || defined(_CFE_) */

#else
#include "rdp_cpu_ring_sim.h"
#endif /* RDP_SIM */

#endif /* _RDP_CPU_RING_H_ */
