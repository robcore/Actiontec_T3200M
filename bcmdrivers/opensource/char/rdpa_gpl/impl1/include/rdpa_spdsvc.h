/*
* <:copyright-BRCM:2014:DUAL/GPL:standard
* 
*    Copyright (c) 2014 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :> 
*/

#ifndef _RDPA_SPDSVC_H_
#define _RDPA_SPDSVC_H_

/** \defgroup spdsvc Speed Service
 * This object is used to manage the Runner Speed Service Generator \n
 * and Analyzer modules.\n
 * @{
 */

/** Speed Service Result.\n
 * The Speed Service Result contains the results provided by the \n
 * Runner Generator and Analyzer at the end of each test run.\n
 */
typedef struct {
    uint8_t running;      /**< 0: Test done; 1: Test in progress */
    uint32_t rx_packets;  /**< Number of packets received by the Analyzer */
    uint32_t rx_bytes;    /**< Number of bytes received by the Analyzer */
    uint32_t tx_packets;  /**< Number of packets transmitted by the Generator */
    uint32_t tx_bytes;    /**< Number of bytes transmitted by the Generator */
} rdpa_spdsvc_result_t;

/** @} end of spdsvc Doxygen group. */

#endif /* _RDPA_SPDSVC_H_ */
