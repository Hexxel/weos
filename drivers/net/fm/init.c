/*
 * Copyright 2011 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <mach/immap_85xx.h>

#include "fm.h"

int fm_standard_init(void)
{
	struct ccsr_fman *reg;

	reg = (void *) MPC85xx_FMAN_FM1_ADDR;
	if (fm_init_common(0, reg))
		return 0;

	return 1;
}

#if 0
/* simple linear search to map from port to array index */
static int fm_port_to_index(enum fm_port port)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fm_info); i++) {
		if (fm_info[i].port == port)
			return i;
	}

	return -1;
}

/*
 * Determine if an interface is actually active based on HW config
 * we expect fman_port_enet_if() to report PHY_INTERFACE_MODE_NONE if
 * the interface is not active based on HW cfg of the SoC
 */
void fman_enet_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fm_info); i++) {
		phy_interface_t enet_if;

		enet_if = fman_port_enet_if(fm_info[i].port);
		if (enet_if != PHY_INTERFACE_MODE_NONE) {
			fm_info[i].enabled = 1;
			fm_info[i].enet_if = enet_if;
		} else {
			fm_info[i].enabled = 0;
		}
	}

	return ;
}

void fm_disable_port(enum fm_port port)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return;

	fm_info[i].enabled = 0;
	fman_disable_port(port);
}

void fm_enable_port(enum fm_port port)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return;

	fm_info[i].enabled = 1;
	fman_enable_port(port);
}

void fm_info_set_mdio(enum fm_port port, struct mii_bus *bus)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return;

	fm_info[i].bus = bus;
}

void fm_info_set_phy_address(enum fm_port port, int address)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return;

	fm_info[i].phy_addr = address;
}

/*
 * Returns the PHY address for a given Fman port
 *
 * The port must be set via a prior call to fm_info_set_phy_address().
 * A negative error code is returned if the port is invalid.
 */
int fm_info_get_phy_address(enum fm_port port)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return -1;

	return fm_info[i].phy_addr;
}

/*
 * Returns the type of the data interface between the given MAC and its PHY.
 * This is typically determined by the RCW.
 */
phy_interface_t fm_info_get_enet_if(enum fm_port port)
{
	int i = fm_port_to_index(port);

	if (i == -1)
		return PHY_INTERFACE_MODE_NONE;

	if (fm_info[i].enabled)
		return fm_info[i].enet_if;

	return PHY_INTERFACE_MODE_NONE;
}

static void
__def_board_ft_fman_fixup_port(void *blob, char * prop, phys_addr_t pa,
				enum fm_port port, int offset)
{
	return ;
}

void board_ft_fman_fixup_port(void *blob, char * prop, phys_addr_t pa,
				enum fm_port port, int offset)
	 __attribute__((weak, alias("__def_board_ft_fman_fixup_port")));

static void ft_fixup_port(void *blob, struct fm_eth_info *info, char *prop)
{
	int off;
	uint32_t ph;
	phys_addr_t paddr = CONFIG_SYS_CCSRBAR_PHYS + info->compat_offset;
	u64 dtsec1_addr = (u64)CONFIG_SYS_CCSRBAR_PHYS +
				CONFIG_SYS_FSL_FM1_DTSEC1_OFFSET;

	off = fdt_node_offset_by_compat_reg(blob, prop, paddr);

	if (info->enabled) {
		fdt_fixup_phy_connection(blob, off, info->enet_if);
		board_ft_fman_fixup_port(blob, prop, paddr, info->port, off);
		return ;
	}

	/* FM1_DTSECx and FM1_10GECx use the same dual-role MAC */
	if (((info->port == FM1_DTSEC1) && (PORT_IS_ENABLED(FM1_10GEC1)))  ||
	    ((info->port == FM1_DTSEC2) && (PORT_IS_ENABLED(FM1_10GEC2)))  ||
	    ((info->port == FM1_DTSEC3) && (PORT_IS_ENABLED(FM1_10GEC3)))  ||
	    ((info->port == FM1_DTSEC4) && (PORT_IS_ENABLED(FM1_10GEC4)))  ||
	    ((info->port == FM1_10GEC1) && (PORT_IS_ENABLED(FM1_DTSEC1)))  ||
	    ((info->port == FM1_10GEC2) && (PORT_IS_ENABLED(FM1_DTSEC2)))  ||
	    ((info->port == FM1_10GEC3) && (PORT_IS_ENABLED(FM1_DTSEC3)))  ||
	    ((info->port == FM1_10GEC4) && (PORT_IS_ENABLED(FM1_DTSEC4)))
		return;

	/* board code might have caused offset to change */
	off = fdt_node_offset_by_compat_reg(blob, prop, paddr);

	/* Don't disable FM1-DTSEC1 MAC as its used for MDIO */
	if (paddr != dtsec1_addr)
		fdt_status_disabled(blob, off); /* disable the MAC node */

	/* disable the fsl,dpa-ethernet node that points to the MAC */
	ph = fdt_get_phandle(blob, off);
	do_fixup_by_prop(blob, "fsl,fman-mac", &ph, sizeof(ph),
		"status", "disabled", strlen("disabled") + 1, 1);
}

void fdt_fixup_fman_ethernet(void *blob)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fm_info); i++)
		ft_fixup_port(blob, &fm_info[i], "fsl,fman-memac");
}
#endif
