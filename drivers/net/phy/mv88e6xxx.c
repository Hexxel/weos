#define pr_fmt(fmt) "mv88e6xxx: " fmt

#include <common.h>
#include <init.h>
#include <mv88e6xxx.h>

#include <linux/phy.h>

#include <net/mdio-smi.h>

#include <libwestermo.h>

#define PCS_100   (0x003d)
#define PCS_1000  (0x003e)
#define PCS_RGMII (0xc000)


struct mv88e6xxx;

struct mv88e6xxx_model {
	const char *name;
	u16 ver;
	u16 mask;
	u32 phy_mask;

	char *smi_mode;

	int num_ports;
	int  (*probe)     (struct mv88e6xxx *sc);
	void (*setup_port)(struct mv88e6xxx *sc, int port);
};

struct mv88e6xxx {
	const struct mv88e6xxx_pdata* pdata;
	const struct mv88e6xxx_model* model;
	struct phy_device *phy;

	struct mii_bus *smi;
	struct mii_bus *smi_phy;
	struct mii_bus *smi_phy_c45;
};

static struct mii_bus *__add_smi_bus(char *driver, int id,
				     struct smi_bus_pdata *pdata)
{
	struct device_d *dev, *bus;

	dev = add_generic_device(driver, id,
				 NULL, 0, 0, 0, pdata);
	if (!dev)
		return NULL;

	device_for_each_child(dev, bus)
		return to_mii_bus(bus);

	return NULL;
}

static int mv88e6xxx_g2_wait(struct mv88e6xxx *sc)
{
	int i, flag;

	flag = mdiobus_read(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD);
	for (i = 0; (flag & MV88E6XXX_G2_SMI_PHY_CMD_BUSY); i++) {
		/*pr_info("g2_smi_phy_cmd_busy\n");*/
		flag = mdiobus_read(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD);
		if (i > 100)
			return 1;
	}

	return 0;
}

#if 0
static int mv88e6xxx_g2_internal_read_c45(struct mv88e6xxx *sc, int port, int dev, int reg)
{
	int cmd1 = MV88E6XXX_G2_SMI_PHY_CMD_BUSY | MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL | MV88E6XXX_G2_SMI_PHY_CMD_OP_45_WRITE_ADDR|((port << 5)& MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK )|(dev & MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK);
	int cmd2 = MV88E6XXX_G2_SMI_PHY_CMD_BUSY | MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL | MV88E6XXX_G2_SMI_PHY_CMD_OP_45_READ_DATA | ((port << 5)& MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK )|(dev & MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK);

	/* pr_info("%s port=%x, dev=%x, reg=%x, cmd1=%x, cmd2=%x\n",__func__, port, dev, reg, cmd1, cmd2);*/
	mv88e6xxx_g2_wait(sc);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_DATA, reg);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD, cmd1);
	mv88e6xxx_g2_wait(sc);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD, cmd2);
	mv88e6xxx_g2_wait(sc);

	return mdiobus_read(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_DATA);
}
#endif

static void mv88e6xxx_g2_internal_write_c45(struct mv88e6xxx *sc, int port, int dev, int reg, int data)
{
	int cmd1 = MV88E6XXX_G2_SMI_PHY_CMD_BUSY | MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL | MV88E6XXX_G2_SMI_PHY_CMD_OP_45_WRITE_ADDR|((port << 5)& MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK )|(dev & MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK);
	int cmd2 = MV88E6XXX_G2_SMI_PHY_CMD_BUSY | MV88E6390_G2_SMI_PHY_CMD_FUNC_INTERNAL | MV88E6XXX_G2_SMI_PHY_CMD_OP_45_WRITE_DATA | ((port << 5)& MV88E6XXX_G2_SMI_PHY_CMD_DEV_ADDR_MASK )|(dev & MV88E6XXX_G2_SMI_PHY_CMD_REG_ADDR_MASK);

	/*pr_info("%s port=%x, dev=%x, reg=%x, data=%x, cmd1=%x, cmd2=%x\n",__func__, port, dev, reg, data, cmd1, cmd2); */
	mv88e6xxx_g2_wait(sc);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_DATA, reg);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD, cmd1);
	mv88e6xxx_g2_wait(sc);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_DATA, data);
	mdiobus_write(sc->smi, MV88E6XXX_G2, MV88E6XXX_G2_SMI_PHY_CMD, cmd2);
	mv88e6xxx_g2_wait(sc);
}

static void mv88e6390x_errata(struct mv88e6xxx *sc)
{
	char cpu = 1;
	char phy = 1;
	char port = 0;
	unsigned short c_mode = 0;

	pr_info("mv88e6390x_errata\n");

	/* Begin Errata workarounds */

	/* Test CPU_Attached mode */
	/* Write Scratch and Misc */
	mdiobus_write(sc->smi, 0x1c, 0x1a, 0x7100);
	/* If Bit 2 = 1, CPU not attached */
	if (mdiobus_read(sc->smi, 0x1c, 0x1a) & 0x4)
		cpu = 0;

	/* Begin PHY recalibration */
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0, 0x1140);
	udelay(1000);
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0x16, 0x00f8);
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0x16, 0x00f8);
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0x8, 0x0036);
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0x16, 0x0000);
	for (phy = 1; phy <= 8; phy ++)
		mdiobus_write(sc->smi_phy, phy, 0x0, 0x9140);

	/* Power down PHY's if in CPU_Attached mode */
	if (cpu)
		for (phy = 0x1; phy <= 0x8; phy ++)
			mdiobus_write(sc->smi_phy, phy, 0x0, 0x1940);
	/* End PHY recalibration */

	/* Begin stuck port issue */
	/* Disable all Ports */
	for (port = 0; port <= 0xa; port ++)
		mdiobus_write(sc->smi, port, 0x4, 0x007c);

	/* Perform Workaround */
	mdiobus_write(sc->smi, 0x5, 0x1a, 0x01c0);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfc00);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfc20);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfc40);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfc60);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfc80);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfca0);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfcc0);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfce0);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfd00);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfd20);
	mdiobus_write(sc->smi, 0x4, 0x1a, 0xfd40);

	/* Restore forwarding state if CPU not attached: */
	if (!cpu)
		for (port = 0x0; port <= 0xa; port++)
			mdiobus_write(sc->smi_phy, port, 0x4, 0x007f);
	/* End Stuck port issue */

	/* Force EEE mode off for P9 and P10 for lower
	   core power when not linked */
	mdiobus_write(sc->smi, 0x9, 0x1, 0x0103);
	mdiobus_write(sc->smi, 0xa, 0x1, 0x0103);

	/* Phase FIFO reset if 10G ports are in XAUI or RXAUI */
	/* Port 9 */
	c_mode = mdiobus_read(sc->smi, 0x9, 0x0) & 0x0f;
	if (c_mode == 0xc || c_mode == 0xd) {
		mv88e6xxx_g2_internal_write_c45(sc, 0x9, 0x4, 0x9000, 0x100);
		mv88e6xxx_g2_internal_write_c45(sc, 0x9, 0x4, 0x9000, 0x0);
	}

	/* Port 10 */
	c_mode = mdiobus_read(sc->smi, 0xa, 0x0) & 0x0f;
	if (c_mode == 0xc || c_mode == 0xd) {
		mv88e6xxx_g2_internal_write_c45(sc, 0xa, 0x4, 0x9000, 0x100);
		mv88e6xxx_g2_internal_write_c45(sc, 0xa, 0x4, 0x9000, 0x0);
	}
	/* End errata */

	/* End Errata Workarounds */
}

static void mv88e6390x_setup_port(struct mv88e6xxx *sc, int port)
{
	u16 host_mode_mask = (1 << sc->pdata->cpu_port);

	pr_info("mv88e6390x port %d setup\n", port);

	/* enable LED */
	mdiobus_write(sc->smi, (port + 0x1), 0x16, 0x5e);
	mdiobus_write(sc->smi, (port + 0x1), 0x16, 0x805e);

	/* enable phy, including PHY recalibration according to peridot errata*/
	mdiobus_write(sc->smi_phy, (port + 1), 0, 0x1140);
	udelay(1000);
	mdiobus_write(sc->smi_phy, (port + 1), 0x16, 0x00F8);
	mdiobus_write(sc->smi_phy, (port + 1), 0x8, 0x0036);
	mdiobus_write(sc->smi_phy, (port + 1), 0x16, 0x0000);
	mdiobus_write(sc->smi_phy, (port + 1), 0, 0x9140);

	/* set switch to forward */
	mdiobus_write(sc->smi, (port + 1), 4, 0x7f);

	/* set switch VLAN */
	mdiobus_write(sc->smi, (port + 1), 6, host_mode_mask);
}

static void mv88e6xxx_setup_port(struct mv88e6xxx *sc, int port)
{
	u16 host_mode_mask = (1 << sc->pdata->cpu_port);

	mdiobus_write(sc->smi_phy, port, 0, 0x9140);
	mdiobus_write(sc->smi, port + 0x10, 4, 0x7f);
	mdiobus_write(sc->smi, port + 0x10, 6, host_mode_mask);
}

static void mv88e60xx_setup_port(struct mv88e6xxx *sc, int port)
{
	mv88e6xxx_setup_port(sc, port);

	/* setup port LEDs */
	mdiobus_write(sc->smi, port, 0x16, 0xaff);
}

static int mv88e6185_probe(struct mv88e6xxx *sc)
{
	struct mv88e6xxx_pdata *pdata = (struct mv88e6xxx_pdata *)sc->pdata;

	pdata->cascade_ports[0] = 9;
	return 0;
}

static int mv88e6352_probe(struct mv88e6xxx *sc)
{
	if (product_is_coronet_cascade()) {
		if (sc->pdata->cascade_ports[0] == 5)
			mdiobus_write(sc->smi, 0x1c, 0x1a, 0xe870); /* 125 MHz clock sc->cpu */
	} else if (product_is_coronet_star()) {
		mdiobus_write(sc->smi, 0x1c, 0x1a, 0xe870); /* 125 MHz clock sc->cpu */

		/* setup SERDES channel */
		mdiobus_write(sc->smi_phy, 0xf, 0x16, 1);   /* set page=1 */
		mdiobus_write(sc->smi_phy, 0xf, 0, 0x8140); /* power up */
		mdiobus_write(sc->smi_phy, 0xf, 26, 0x47);  /* output power: 700mV */
	} else {
		if (sc->pdata->cascade_ports[0] == 4) {
			mdiobus_write(sc->smi, 0x1c, 0x1a, 0xe870); /* 125 MHz clock sc->cpu */
			mdiobus_write(sc->smi, 0x14, 0, 0);         /* clear PHY reg bit */

			/* setup SERDES channel */
			mdiobus_write(sc->smi_phy, 0xf, 0x16, 1);   /* set page=1 */
			mdiobus_write(sc->smi_phy, 0xf, 0, 0x8140); /* power up */
			mdiobus_write(sc->smi_phy, 0xf, 26, 0x47);  /* output power: 700mV */
		}
	}

	return 0;
}

static int mv88e6390x_probe(struct mv88e6xxx *sc)
{
	/* mv88e6390x errata fixes */
	mv88e6390x_errata(sc);

	/* GPIO Pin Control 3 of Scratch/misc register: 125 MHz clock sc->cpu */
	mdiobus_write(sc->smi, 0x1c, 0x1a, 0xeb70);

	pr_info("mv88e6390x probed\n");

	return 0;
}

const static struct mv88e6xxx_model models[] = {
	{
		.name = "88E6046",
		.ver  = 0x0480,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 4,
		.phy_mask    = 0xf,
		.setup_port  = mv88e60xx_setup_port,
	},
	{
		.name = "88E6097",
		.ver  = 0x0990,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 8,
		.phy_mask    = 0xff,
		.setup_port  = mv88e60xx_setup_port,
	},
	{
		.name = "88E6185",
		.ver  = 0x1a70,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 8,
		.phy_mask    = 0xff,
		.probe       = mv88e6185_probe,
	},
	{
		.name = "88E6352",
		.ver  = 0x3520,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-indirect",

		.num_ports   = 5,
		.phy_mask    = 0xff,
		.probe       = mv88e6352_probe,
		.setup_port  = mv88e6xxx_setup_port,
	},
	{
		.name = "88E6390x",
		.ver  = 0x0A10,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-indirect",

		.num_ports   = 8,
		.phy_mask    = 0x1fe,
		.probe       = mv88e6390x_probe,
		.setup_port  = mv88e6390x_setup_port,
	},
	{ .ver = 0 }
};

static int mv88e6xxx_config_init(struct phy_device *phydev)
{
	struct mv88e6xxx *sc = phydev->priv;
	u16 pcs = PCS_RGMII | PCS_1000;
	u16 gc;
	int i;

	if (sc->model->setup_port) {
		if (product_is_coronet_tbn()) {
			/* Workaround stuck port issue on peridot cf. Marvell doc MV-S302624-00 */
			pr_err("workaround stuck port\n");
			mdiobus_write(sc->smi, 5, 0x1A, 0x01C0);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFC00);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFC20);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFC40);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFC60);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFC80);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFCA0);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFCC0);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFCE0);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFD00);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFD20);
			mdiobus_write(sc->smi, 4, 0x1A, 0xFD40);
			gc = 0x8000 | mdiobus_read(sc->smi, 0x1B, 4);
			mdiobus_write(sc->smi, 0x1B, 4, gc); /*SW_RSET*/
			/* end stuck port issue*/
		}

		for (i = 0; i < sc->model->num_ports; i++)
			sc->model->setup_port(sc, i);
	}

	if (sc->pdata->cpu_speed == 100)
		pcs = PCS_100;
	if (product_is_coronet_tbn()) {
		mdiobus_write(sc->smi, sc->pdata->cpu_port, 1, pcs);
		mdiobus_write(sc->smi, sc->pdata->cpu_port, 4, 0x7f);
	}
	else {
		mdiobus_write(sc->smi, sc->pdata->cpu_port + 0x10, 1, pcs);
		mdiobus_write(sc->smi, sc->pdata->cpu_port + 0x10, 4, 0x7f);
	}
	if (sc->pdata->cascade_ports[0] != -1) {
		mdiobus_write(sc->smi, sc->pdata->cascade_ports[0] + 0x10, 1,
				sc->pdata->cascade_rgmii[0] ? pcs : PCS_1000);
		mdiobus_write(sc->smi, sc->pdata->cascade_ports[0] + 0x10, 4, 0x7f);
	}

	for (i = 0; sc->pdata->slaves[i]; i++)
		phy_init_hw(sc->pdata->slaves[i]);

	phydev->speed  = sc->pdata->cpu_speed;
	phydev->duplex = 1;
	phydev->link   = 1;
	if (phydev->attached_dev && phydev->adjust_link)
		phydev->adjust_link(phydev->attached_dev);
	return 0;
}

static void mv88e6xxx_remove(struct phy_device *phydev)
{
	struct mv88e6xxx *sc = phydev->priv;
	int i;

	for (i = 0; i < sc->model->num_ports; i++) {
		if (product_is_coronet_tbn()) {
			mdiobus_write(sc->smi_phy, (i + 1), 0, 0x8800);
			/* Reset port based vlan map */
			mdiobus_write(sc->smi, (i + 1), 6, 0x7ff & ~(1 << (i + 1)));
		}
		else {
		        mdiobus_write(sc->smi_phy, i, 0, 0x8800);
			/* Reset port based vlan map */
			mdiobus_write(sc->smi, i + 0x10, 6, 0x7ff & ~(1 << i));
		}
	}
	mdiobus_write(sc->smi, 0x1c, 4, 0x8000);
}

static int mv88e6xxx_probe(struct phy_device *phydev)
{
	struct mv88e6xxx *sc = xzalloc(sizeof(*sc));
	struct smi_bus_pdata smi_phy_data;
	struct smi_bus_pdata smi_data = {
		.addr = phydev->addr,
		.bus  = phydev->bus,
	};
	u16 ver;
	int i;

	pr_info("%s Probe\n", __func__);
	if (!phydev->priv) {
		pr_err("error, no platform data\n");
		return -EINVAL;
	}

	sc->pdata = phydev->priv;
	phydev->priv = sc;
	sc->phy = phydev;

	sc->smi = __add_smi_bus("mdio-smi", phydev->addr, &smi_data);
	if (!sc->smi) {
		pr_err("error, unable to create smi bus\n");
		return -ENODEV;
	}

	if (product_is_coronet_tbn())
		ver = mdiobus_read(sc->smi, 0x0, 3);
	else
		ver = mdiobus_read(sc->smi, 0x10, 3);

	for (i = 0; models[i].ver; i++) {
		if ((ver & models[i].mask) == models[i].ver) {
			sc->model = &models[i];
			pr_info("found chip, model:%s\n", sc->model->name);
			break;
		}
	}

	if (!sc->model) {
		pr_err("error, unsupported model:%#.4x\n", ver);
		return -ENOSYS;
	}

	smi_phy_data.bus = sc->smi;
	sc->smi_phy->phy_mask = sc->model->phy_mask;
	sc->smi_phy = __add_smi_bus(sc->model->smi_mode, sc->phy->addr, &smi_phy_data);
	if (!sc->smi_phy)
		return -ENODEV;

	if (product_is_coronet_tbn()) {
/*		sc->smi_phy_c45 = __add_smi_bus("mdio-smi-indirect_c45",
						sc->phy->addr, &smi_phy_data);
		if (!sc->smi_phy_c45)
			return -ENODEV;
*/
		sc->phy->phy_isC45 = 0;
		sc->phy->phy_isC22overC45 = 1;
	}

	if (sc->model->probe)
		return sc->model->probe(sc);
	else
		return 0;

	pr_info("%s ok!\n", __func__);
}

static struct phy_driver mv88e6xxx_driver = {
	.phy_id		= 0x88e60000,
	.phy_id_mask	= 0xffff0000,
	.drv.name	= "mv88e6xxx",
	.features	= PHY_GBIT_FEATURES,
	.probe		= mv88e6xxx_probe,
	.remove		= mv88e6xxx_remove,
	.config_init	= mv88e6xxx_config_init,
};

static int mv88e6xxx_phy_init(void)
{
	return phy_driver_register(&mv88e6xxx_driver);
}
fs_initcall(mv88e6xxx_phy_init);
