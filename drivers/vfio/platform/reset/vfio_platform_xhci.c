/*
 * VFIO platform driver specialized for XHCI reset
 * reset code is inherited from XHCI native driver
 *
 * Copyright 2016 Marvell Semiconductors, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/phy/phy.h>
#include <linux/usb/phy.h>

#include "vfio_platform_private.h"

#define DRIVER_VERSION  "0.1"
#define DRIVER_AUTHOR   "Yehuda Yitschak <yehuday@marvell.com>"
#define DRIVER_DESC     "Reset support for XHCI vfio platform device"

#define MAX_XHCI_CLOCKS		4
#define MAX_XHCI_PHYS		2

int vfio_platform_xhci_reset(struct vfio_platform_device *vdev)
{
	struct device *dev = vdev->device;
	struct device_node *np = dev->of_node;
	struct clk *clk;
	struct phy *phy;
	struct usb_phy *usb_phy;
	struct regulator *current_limiter_regulator;
	int ret, i;

	for (i = 0; i < MAX_XHCI_CLOCKS; i++) {
		clk = of_clk_get(np, i);
		if (!IS_ERR(clk)) {
			ret = clk_prepare_enable(clk);
			if (ret)
				return -ENODEV;
		}
	}

	for (i = 0; i < MAX_XHCI_PHYS; i++) {
		phy = devm_of_phy_get_by_index(dev, np, i);
		if (!IS_ERR(phy)) {
			ret = phy_power_on(phy);
			if (ret)
				return -ENODEV;
		}
	}

	usb_phy = devm_usb_get_phy_by_phandle(dev, "usb-phy", 0);
	if (!IS_ERR(usb_phy)) {
		ret = usb_phy_init(usb_phy);
		if (ret)
			return -ENODEV;
		current_limiter_regulator =
			devm_regulator_get_optional(usb_phy->dev,
						    "current-limiter");
		if (!IS_ERR(current_limiter_regulator)) {
			if (regulator_enable(current_limiter_regulator))
				dev_err(dev,
					"Failed to enable Current-limiter regulator\n");
		}

	}

	return 0;
}

module_vfio_reset_handler("generic-xhci", vfio_platform_xhci_reset);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
