/*
 * RealTek PHY drivers
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 * author Andy Fleming
 */
#include <config.h>
#include <common.h>
#include <init.h>

#include <linux/phy.h>

#define PHY_AUTONEGOTIATE_TIMEOUT 5000

/* RTL8211x PHY Status Register */
#define MIIM_RTL8211x_PHY_STATUS       0x11
#define MIIM_RTL8211x_PHYSTAT_SPEED    0xc000
#define MIIM_RTL8211x_PHYSTAT_GBIT     0x8000
#define MIIM_RTL8211x_PHYSTAT_100      0x4000
#define MIIM_RTL8211x_PHYSTAT_DUPLEX   0x2000
#define MIIM_RTL8211x_PHYSTAT_SPDDONE  0x0800
#define MIIM_RTL8211x_PHYSTAT_LINK     0x0400

static int rtl8211x_read_status(struct phy_device *phydev)
{
	unsigned int speed;
	unsigned int mii_reg;

	genphy_update_link(phydev);

	mii_reg = phy_read(phydev, MIIM_RTL8211x_PHY_STATUS);

	if (!(mii_reg & MIIM_RTL8211x_PHYSTAT_SPDDONE)) {
		int i = 0;

		/* in case of timeout ->link is cleared */
		phydev->link = 1;
		while (!(mii_reg & MIIM_RTL8211x_PHYSTAT_SPDDONE)) {
			/* Timeout reached ? */
			if (i > PHY_AUTONEGOTIATE_TIMEOUT) {
				phydev->link = 0;
				break;
			}

			udelay(1000);	/* 1 ms */
			mii_reg = phy_read(phydev, MIIM_RTL8211x_PHY_STATUS);
		}
		udelay(500000);	/* another 500 ms (results in faster booting) */
	} else {
		if (mii_reg & MIIM_RTL8211x_PHYSTAT_LINK)
			phydev->link = 1;
		else
			phydev->link = 0;
	}

	if (mii_reg & MIIM_RTL8211x_PHYSTAT_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	speed = (mii_reg & MIIM_RTL8211x_PHYSTAT_SPEED);

	switch (speed) {
	case MIIM_RTL8211x_PHYSTAT_GBIT:
		phydev->speed = SPEED_1000;
		break;
	case MIIM_RTL8211x_PHYSTAT_100:
		phydev->speed = SPEED_100;
		break;
	default:
		phydev->speed = SPEED_10;
	}

	return 0;
}

static struct phy_driver rtl8211b_driver = {
	.drv.name = "RealTek RTL8211B",
	.phy_id = 0x1cc910,
	.phy_id_mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.read_status = rtl8211x_read_status,
};

static struct phy_driver rtl8211e_driver = {
	.drv.name = "RealTek RTL8211E",
	.phy_id = 0x1cc915,
	.phy_id_mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.read_status = rtl8211x_read_status,
};

static struct phy_driver rtl8211dn_driver = {
	.drv.name = "RealTek RTL8211DN",
	.phy_id = 0x1cc914,
	.phy_id_mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.read_status = rtl8211x_read_status,
};

static int ns_phy_init(void)
{
	phy_driver_register(&rtl8211b_driver);
	phy_driver_register(&rtl8211e_driver);
	phy_driver_register(&rtl8211dn_driver);
	return 0;
}
fs_initcall(ns_phy_init);
