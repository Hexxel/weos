
#include <common.h>
#include <command.h>
#include <getopt.h>

#include <linux/phy.h>

static char *rmon_strings[2][32] = {
{
	"InGoodOctetsLo",
	"InGoodOctetsHi",
	"InBadOctets",
	"OutFCSErr",
	"InUnicast",
	"Deferred",
	"InBroadcasts",
	"InMulticasts",
	"64Octets",
	"65to127Octets",
	"128to256Octets",
	"256to511Octets",
	"512to1023Octets",
	"MaxOctets",
	"OutOctetsLo",
	"OutOctetsHi",
	"OutUnicast",
	"Excessive",
	"OutMulticasts",
	"OutBroadcasts",
	"Single",
	"OutPause",
	"InPause",
	"Multiple",
	"InUndersize",
	"InFragments",
	"InOversize",
	"InJabber",
	"InRxErr",
	"InFCSErr",
	"Collisions",
	"Late"
},
{
	"InDiscards",
	"InFiltered",
	"InAccepted",
	"InBadAccepted",
	"InGoodAvbClassA",
	"InGoodAvbClassB",
	"InBadAvbClassA",
	"InBadAvbClassB",
	"TcamCounter0",
	"TcamCounter1",
	"TcamCounter2",
	"TcamCounter3",
	"InDaUnknown",
	"InManagement",
	"OutQueue0",
	"OutQueue1",
	"OutQueue2",
	"OutQueue3",
	"OutQueue4",
	"OutQueue5",
	"OutQueue6",
	"OutQueue7",
	"OutCutThrough",
	"OutOctetsA",
	"OutOctetsB",
	"-",
	"-",
	"-",
	"-",
	"-",
	"-",
	"-"
}
};


enum mvops {
	MV_NONE,
	MV_RMON_FLUSH,
	MV_RMON_DUMP,
	MV_ATU,
	MV_VTU,
};

static int is_atu_banked(struct mii_bus *bus)
{
   int product_num = 0;

   product_num = mdiobus_read(bus, ADDRESS_CHIP_VERSION, REGISTER_CHIP_VERSION);
   if ((product_num & 0xfff0) == M88E6390)
   {
      return 1;
   }
   return 0;
}

static int rmon_op (struct mii_bus *bus, int port, enum mvops op, int mode, int bank)
{
	int i = 0;

	for (; i <= 0x1f; i++) {
		uint16_t val = (1 << 15) | mode | (port << 5);

		if (op == MV_RMON_DUMP)
			val |= (0x5 << 12);
		else if (op == MV_RMON_FLUSH)
			val |= (0x2 << 12);
		else
			return 1;

		mdiobus_write(bus, 0x1b, 0x1d, val);
		while (mdiobus_read(bus, 0x1b, 0x1d) & 0x8000);

		if (op == MV_RMON_DUMP) {
			val = (1 << 15) | (0x4 << 12) | mode | (port << 5);

			mdiobus_write(bus, 0x1b, 0x1d, val | i);
			while (mdiobus_read(bus, 0x1b, 0x1d) & 0x8000);
			pr_err("%s: %d\n", rmon_strings[bank][i], mdiobus_read(bus, 0x1b, 0x1f) | mdiobus_read(bus, 0x1b, 0x1e) << 16);
		}
	}
	return 0;
}

static int do_rmon_op (struct mii_bus *bus, int port, enum mvops op)
{
	if (is_atu_banked(bus)) {
		rmon_op (bus, port, op, 0, 0);
		rmon_op (bus, port, op, (1 << 10), 1);
	}
	else {
		rmon_op (bus, port, op, (3 << 10), 0);
	}

	return 0;
}


static int do_mvcfg(int argc, char *argv[])
{
	struct mii_bus *bus;
	char *bus_name = NULL;
	int port, opt;
	enum mvops op;

	while ((opt = getopt(argc, argv, "b:r:")) > 0) {
		switch (opt) {
		case 'b':
			bus_name = optarg;
			break;

		case 'r':
			if (!strcmp(optarg, "flush"))
				op = MV_RMON_FLUSH;
			else if (!strcmp(optarg, "dump"))
				op = MV_RMON_DUMP;
			else
				return COMMAND_ERROR_USAGE;

			if (argc < 6)
				return COMMAND_ERROR_USAGE;
			port = simple_strtol(argv[optind], NULL, 0);
			port++;
			break;

		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!bus_name || !op)
		return COMMAND_ERROR_USAGE;

	for_each_mii_bus(bus) {
		if (!bus_name || !strcmp(bus_name, dev_name(&bus->dev)) ||
		    !strcmp(bus_name, dev_name(bus->parent))) {
			if (op == MV_RMON_DUMP || op == MV_RMON_FLUSH)
				return do_rmon_op(bus, port, op);
		}
	}
	return -ENODEV;
}

BAREBOX_CMD_START(mvcfg)
	.cmd		= do_mvcfg,
	BAREBOX_CMD_DESC("Control a Marvel SOHO chip.")
	BAREBOX_CMD_OPTS("-b <miibus> -r <dump|flush> port")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
