/**
 * Copyright (c) 2012-2013 Quantenna Communications, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

/*
 * Platform dependant implement. Customer needs to modify this file.
 */
#include <qdpc_platform.h>
#include <topaz_vnet.h>
#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>

static int vmac_rst_rc_en = 1;
struct work_struct detect_ep_rst_work;

void enable_vmac_ints(struct vmac_priv *vmp)
{
	uint32_t temp = readl(QDPC_RC_SYS_CTL_PCIE_INT_MASK);

	if(vmp->msi_enabled) {
		temp |= BIT(10); /* MSI */
	} else {
		temp |= BIT(11); /* Legacy INTx */
	}
	writel(temp, QDPC_RC_SYS_CTL_PCIE_INT_MASK);
}

void disable_vmac_ints(struct vmac_priv *vmp)
{
	uint32_t temp = readl(QDPC_RC_SYS_CTL_PCIE_INT_MASK);

	if(vmp->msi_enabled) {
		temp &= ~BIT(10); /* MSI */
	} else {
		temp &= ~BIT(11); /* Legacy INTx */
	}
	writel(temp, QDPC_RC_SYS_CTL_PCIE_INT_MASK);
}

static ssize_t vmac_reset_get(struct device *dev, struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", vmac_rst_rc_en);
}

static ssize_t vmac_reset_set(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
        uint8_t cmd;

        cmd = (uint8_t)simple_strtoul(buf, NULL, 10);
	if (cmd == 0)
		vmac_rst_rc_en = 0;
	else
		vmac_rst_rc_en = 1;

        return count;
}
DEVICE_ATTR(enable_reset, S_IWUSR | S_IRUSR, vmac_reset_get, vmac_reset_set);

static void detect_ep_rst(struct work_struct *data)
{
	kernel_restart(NULL);
}

void enable_ep_rst_detection(struct net_device *ndev)
{
        uint32_t temp = readl(QDPC_RC_SYS_CTL_PCIE_INT_MASK);

        temp |= QDPC_INTR_EP_RST_MASK;
        writel(temp, QDPC_RC_SYS_CTL_PCIE_INT_MASK);

	device_create_file(&ndev->dev, &dev_attr_enable_reset);
	INIT_WORK(&detect_ep_rst_work, detect_ep_rst);
}

void disable_ep_rst_detection(struct net_device *ndev)
{
        uint32_t temp = readl(QDPC_RC_SYS_CTL_PCIE_INT_MASK);

        temp &= ~QDPC_INTR_EP_RST_MASK;
        writel(temp, QDPC_RC_SYS_CTL_PCIE_INT_MASK);

	device_remove_file(&ndev->dev, &dev_attr_enable_reset);
}

void handle_ep_rst_int(struct net_device *ndev)
{
	uint32_t status = readl(QDPC_RC_SYS_CTL_PCIE_INT_STAT);

	if ((status & QDPC_INTR_EP_RST_MASK) == 0)
		return;

	/* Clear pending interrupt */
	writel(QDPC_INTR_EP_RST_MASK, QDPC_RC_SYS_CTL_PCIE_INT_STAT);

	printk("Detected reset of Endpoint\n");

	if (vmac_rst_rc_en == 1) {
		netif_stop_queue(ndev);
		schedule_work(&detect_ep_rst_work);
	}
}

