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

const static struct mv88e6xxx_model models[] = {
	{
		.name = "88E6046",
		.ver  = 0x0480,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 4,
		.setup_port  = mv88e60xx_setup_port,
	},
	{
		.name = "88E6097",
		.ver  = 0x0990,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 8,
		.setup_port  = mv88e60xx_setup_port,
	},
	{
		.name = "88E6185",
		.ver  = 0x1a70,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-direct",

		.num_ports   = 8,
		.probe       = mv88e6185_probe,
	},
	{
		.name = "88E6352",
		.ver  = 0x3520,
		.mask = 0xfff0,

		.smi_mode = "mdio-smi-indirect",

		.num_ports   = 5,
		.probe       = mv88e6352_probe,
		.setup_port  = mv88e6xxx_setup_port,
	},

	{ .ver = 0 }
};

static int mv88e6xxx_config_init(struct phy_device *phydev)
{
	struct mv88e6xxx *sc = phydev->priv;
	u16 pcs = PCS_RGMII | PCS_1000;
	int i;

	if (sc->model->setup_port)
		for (i = 0; i < sc->model->num_ports; i++)
			sc->model->setup_port(sc, i);

	if (sc->pdata->cpu_speed == 100)
		pcs = PCS_100;

	mdiobus_write(sc->smi, sc->pdata->cpu_port + 0x10, 1, pcs);
	mdiobus_write(sc->smi, sc->pdata->cpu_port + 0x10, 4, 0x7f);

	mdiobus_write(sc->smi, sc->pdata->cascade_ports[0] + 0x10, 1, sc->pdata->cascade_rgmii[0] ? pcs : PCS_1000);
	mdiobus_write(sc->smi, sc->pdata->cascade_ports[0] + 0x10, 4, 0x7f);

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
		mdiobus_write(sc->smi_phy, i, 0, 0x8800);
		/* Reset port based vlan map */
		mdiobus_write(sc->smi, i + 0x10, 6, 0x7ff & ~(1 << i));
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

   pr_err("%s: Entering\n", __func__);
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
	sc->smi_phy = __add_smi_bus(sc->model->smi_mode, sc->phy->addr,
				    &smi_phy_data);
	if (!sc->smi_phy)
		return -ENODEV;

	if (sc->model->probe)
		return sc->model->probe(sc);
	else
		return 0;
}

static struct phy_driver mv88e6xxx_driver = {
	.phy_id		= 0x88e60000,
	.phy_id_mask	= 0xffff0000,
	.drv.name	= "mv88e6xxx",
	.features	= PHY_GBIT_FEATURES,
	.probe		= mv88e6xxx_probe,
        .remove         = mv88e6xxx_remove,
	.config_init    = mv88e6xxx_config_init,
};

static int mv88e6xxx_phy_init(void)
{
	return phy_driver_register(&mv88e6xxx_driver);
}
fs_initcall(mv88e6xxx_phy_init);
