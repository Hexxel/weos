/*
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <getopt.h>

#include <linux/phy.h>

static int do_mdio_op(struct mii_bus *bus, int addr, int reg, int val)
{
	if (val != -1)
		return mdiobus_write(bus, addr, reg, val);

	val = mdiobus_read(bus, addr, reg);
	if (val < 0)
		return val;

	printf("0x%.4x\n", val);
	return COMMAND_SUCCESS;
}

static int do_mdio(int argc, char *argv[])
{
	struct mii_bus *bus;
	char *bus_name = NULL;
	int opt, addr, reg, val = -1;

	while ((opt = getopt(argc, argv, "b:")) > 0) {
		switch (opt) {
		case 'b':
			bus_name = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind < 2)
		return COMMAND_ERROR_USAGE;
	
	addr = simple_strtol(argv[optind],     NULL, 0);
	reg  = simple_strtol(argv[optind + 1], NULL, 0);

	if (argc - optind > 2)
		val = simple_strtol(argv[optind + 2], NULL, 0);

	for_each_mii_bus(bus) {
		if (!bus_name || !strcmp(bus_name, dev_name(&bus->dev)) ||
		    !strcmp(bus_name, dev_name(bus->parent)))
			return do_mdio_op(bus, addr, reg, val);
	}

	return -ENODEV;
}

BAREBOX_CMD_START(mdio)
	.cmd		= do_mdio,
	BAREBOX_CMD_DESC("raw read/write access to an MDIO bus.")
	BAREBOX_CMD_OPTS("[-b <bus>] <addr> <reg> [<val>]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
