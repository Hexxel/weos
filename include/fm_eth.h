/*
 * Copyright 2009-2012 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FM_ETH_H__
#define __FM_ETH_H__

#include <common.h>
#include <linux/phy.h>
#include <asm/types.h>

enum fm_port {
	FM1_DTSEC1,
	FM1_DTSEC2,
	FM1_DTSEC3,
	FM1_DTSEC4,
	FM1_DTSEC5,
	FM1_DTSEC6,
	FM1_DTSEC9,
	FM1_DTSEC10,
	FM1_10GEC1,
	FM1_10GEC2,
	FM1_10GEC3,
	FM1_10GEC4,
	FM2_DTSEC1,
	FM2_DTSEC2,
	FM2_DTSEC3,
	FM2_DTSEC4,
	FM2_DTSEC5,
	FM2_DTSEC6,
	FM2_DTSEC9,
	FM2_DTSEC10,
	FM2_10GEC1,
	FM2_10GEC2,
	NUM_FM_PORTS,
};

enum fm_eth_type {
	FM_ETH_1G_E,
	FM_ETH_10G_E,
};

/* Fman ethernet info struct */
#define FM_ETH_INFO_INITIALIZER(idx, pregs) \
	.fm		= idx,						\

#define FM_DTSEC_INFO_INITIALIZER(idx, n) \
{									\
	FM_ETH_INFO_INITIALIZER(idx, 0)					\
	.index		= idx,						\
	.num		= n - 1,					\
	.type		= FM_ETH_1G_E,					\
	.port		= FM##idx##_DTSEC##n,				\
	.rx_port_id	= RX_PORT_1G_BASE + n - 1,			\
	.tx_port_id	= TX_PORT_1G_BASE + n - 1,			\
	.compat_offset	= MPC85xx_FMAN_FM##idx##_OFFSET +		\
				offsetof(struct ccsr_fman, memac[n-1]),	\
	.enet_if	= PHY_INTERFACE_MODE_SGMII,			\
}

struct fm_eth_info {
	u8 enabled;
	u8 fm;
	u8 num;
	u8 phy_addr;
	int index;
	u16 rx_port_id;
	u16 tx_port_id;
	enum fm_port port;
	enum fm_eth_type type;
	void *phy_regs;
	phy_interface_t enet_if;
	u32 compat_offset;
	struct mii_bus *bus;
	void __iomem *regs;
};

int fm_standard_init(void);
void fman_enet_init(void);
void fdt_fixup_fman_ethernet(void *fdt);
phy_interface_t fm_info_get_enet_if(enum fm_port port);
void fm_info_set_phy_address(enum fm_port port, int address);
int fm_info_get_phy_address(enum fm_port port);
void fm_info_set_mdio(enum fm_port port, struct mii_bus *bus);
void fm_disable_port(enum fm_port port);
void fm_enable_port(enum fm_port port);
void set_sgmii_phy(struct mii_bus *bus, enum fm_port base_port,
		unsigned int port_num, int phy_base_addr);
int is_qsgmii_riser_card(struct mii_bus *bus, int phy_base_addr,
		unsigned int port_num, unsigned regnum);

#endif
