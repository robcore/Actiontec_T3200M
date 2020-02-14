/*SH0
*******************************************************************************
**                                                                           **
**         Copyright (c) 2011-2012 Quantenna Communications, Inc             **
**                            All Rights Reserved                            **
**                                                                           **
*******************************************************************************
**                                                                           **
**  Redistribution and use in source and binary forms, with or without       **
**  modification, are permitted provided that the following conditions       **
**  are met:                                                                 **
**  1. Redistributions of source code must retain the above copyright        **
**     notice, this list of conditions and the following disclaimer.         **
**  2. Redistributions in binary form must reproduce the above copyright     **
**     notice, this list of conditions and the following disclaimer in the   **
**     documentation and/or other materials provided with the distribution.  **
**  3. The name of the author may not be used to endorse or promote products **
**     derived from this software without specific prior written permission. **
**                                                                           **
**  Alternatively, this software may be distributed under the terms of the   **
**  GNU General Public License ("GPL") version 2, or (at your option) any    **
**  later version as published by the Free Software Foundation.              **
**                                                                           **
**  In the case this software is distributed under the GPL license,          **
**  you should have received a copy of the GNU General Public License        **
**  along with this software; if not, write to the Free Software             **
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  **
**                                                                           **
**  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR       **
**  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES**
**  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  **
**  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,         **
**  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT **
**  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,**
**  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    **
**  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      **
**  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF **
**  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.        **
**                                                                           **
*******************************************************************************
EH0*/

#ifndef __TOPAZ_PCIE_TQE_H
#define __TOPAZ_PCIE_TQE_H

#include <qtn/topaz_tqe_cpuif.h>
#include <qtn/topaz_fwt_db.h>

#ifdef PCIE_TQE_INTR_WORKAROUND
#define TOPAZ_TQE_PCIE_REL_PORT	TOPAZ_TQE_DSP_PORT
#else
#define TOPAZ_TQE_PCIE_REL_PORT	TOPAZ_TQE_PCIE_PORT
#endif

#define VMAC_PCIE_PORT_ID	TOPAZ_TQE_PCIE_REL_PORT
#define topaz_tqe_pcieif_descr topaz_tqe_cpuif_descr

RUBY_INLINE int
topaz_tqe_pcieif_ppctl_write(const union topaz_tqe_cpuif_ppctl *ctl)
{
	return __topaz_tqe_cpuif_ppctl_write(TOPAZ_TQE_PCIE_REL_PORT, ctl);
}

RUBY_INLINE void
__topaz_tqe_pcieif_tx_start(enum topaz_tqe_port port, const union topaz_tqe_cpuif_ppctl *ctl)
{
	int num = __topaz_tqe_cpuif_ppctl_write(port, ctl);
	qtn_mproc_sync_mem_write(TOPAZ_TQE_CPUIF_TXSTART(num), TOPAZ_TQE_CPUIF_TX_START_NREADY);
}

RUBY_INLINE void
topaz_tqe_pcieif_tx_start(const union topaz_tqe_cpuif_ppctl *ctl)
{
	__topaz_tqe_pcieif_tx_start(TOPAZ_TQE_PCIE_REL_PORT, ctl);
}

RUBY_INLINE int
__topaz_tqe_pcieif_tx_nready(enum topaz_tqe_port port)
{
	int num = topaz_tqe_cpuif_port_to_num(port);
	return (qtn_mproc_sync_mem_read(TOPAZ_TQE_CPUIF_TXSTART(num)) &
		TOPAZ_TQE_CPUIF_TX_START_NREADY);
}

RUBY_INLINE int
topaz_tqe_pcieif_tx_nready(void)
{
	return __topaz_tqe_pcieif_tx_nready(TOPAZ_TQE_PCIE_REL_PORT);
}

RUBY_INLINE void
topaz_tqe_pcieif_setup_reset(int reset)
{
	__topaz_tqe_cpuif_setup_reset(TOPAZ_TQE_PCIE_REL_PORT, reset);
}

RUBY_INLINE void
topaz_tqe_pcieif_setup_ring(union topaz_tqe_cpuif_descr *base, uint16_t count)
{
	__topaz_tqe_cpuif_setup_ring(TOPAZ_TQE_PCIE_REL_PORT, base, count);
}

RUBY_INLINE union topaz_tqe_cpuif_descr*
topaz_tqe_pcieif_get_curr(void)
{
	return __topaz_tqe_cpuif_get_curr(TOPAZ_TQE_PCIE_REL_PORT);
}


RUBY_INLINE void
topaz_tqe_pcieif_put_back(union topaz_tqe_cpuif_descr * descr)
{
	__topaz_tqe_cpuif_put_back(TOPAZ_TQE_PCIE_REL_PORT, descr);
}


RUBY_INLINE union topaz_tqe_cpuif_status
topaz_tqe_pcieif_get_status(void)
{
	return __topaz_tqe_cpuif_get_status(TOPAZ_TQE_PCIE_REL_PORT);
}

int topaz_pcie_tqe_xmit(fwt_db_entry *fwt_ent, void *data_bus, int data_len);
fwt_db_entry *vmac_get_tqe_ent(const unsigned char *src_mac_be, const unsigned char *dst_mac_be);
struct net_device * tqe_pcie_netdev_init( struct net_device *pcie_ndev);

#endif

