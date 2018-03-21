#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>

#include <linux/phy.h>
#include <net/mdio-smi.h>

struct smi_bus {
	u8 addr;

	struct mii_bus  bus;
	struct mii_bus *mii;
};

static int smi_wait_ready(struct mii_bus *bus)
{
	struct smi_bus *smi = bus->priv;
	int ret, i;

	for (i = 0; i < 16; i++) {
		ret = smi->mii->read(smi->mii, smi->addr, 0);
		if (ret < 0)
			return ret;

		if ((ret & 0x8000) == 0)
			return 0;
	}

	return -ETIMEDOUT;
}

int smi_read(struct mii_bus *bus, int addr, int reg)
{
	struct smi_bus *smi = bus->priv;
	int ret;

	ret = smi_wait_ready(bus);
	if (ret < 0)
		goto out;

	ret = smi->mii->write(smi->mii, smi->addr,
			      0, 0x9800 | (addr << 5) | reg);
	if (ret < 0)
		goto out;

	ret = smi_wait_ready(bus);
	if (ret < 0)
		goto out;

	ret = smi->mii->read(smi->mii, smi->addr, 1);

out:
	/* pr_info("%s read(%#x, %#x): %#x\n", bus->id, addr, reg, ret); */
	return ret;
}

int smi_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct smi_bus *smi = bus->priv;
	int ret;

	/* pr_info("%s write(%#x, %#x, %#x)\n", bus->id, addr, reg, val); */

	ret = smi_wait_ready(bus);
	if (ret < 0)
		goto out;

	ret = smi->mii->write(smi->mii, smi->addr, 1, val);
	if (ret < 0)
		goto out;

	ret = smi->mii->write(smi->mii, smi->addr,
			      0, 0x9400 | (addr << 5) | reg);
	if (ret < 0)
		goto out;

	ret = smi_wait_ready(bus);

out:
	return ret;
}

static int smi_reset(struct mii_bus *bus)
{
	return 0;
}

static int smi_bus_probe(struct device_d *dev)
{
	struct smi_bus_pdata *pdata = dev->platform_data;
	struct smi_bus *smi;
	int ret;

	smi = xzalloc(sizeof(*smi));

	smi->addr = pdata->addr;
	smi->mii  = pdata->bus;

	smi->bus.read   = smi_read;
	smi->bus.write  = smi_write;
	smi->bus.reset  = smi_reset;
	smi->bus.priv   = smi;
	smi->bus.parent = dev;
	dev->priv = smi;

	ret = mdiobus_register(&smi->bus);
	if (ret)
		return ret;

	return 0;
}

static struct driver_d smi_bus_driver = {
	.name  = "mdio-smi",
	.probe = smi_bus_probe,
};
register_driver_macro(coredevice, platform, smi_bus_driver);


static inline int smi_ppu_save_disable(struct mii_bus *smi_bus)
{
	uint16_t gc;

	gc = smi_read(smi_bus, SMI_G1, SMI_G1_GC);
	if (gc & SMI_G1_GC_PPU)
	{
		smi_write(smi_bus, SMI_G1, SMI_G1_GC, gc & ~SMI_G1_GC_PPU);

		while (smi_read(smi_bus, SMI_G1, SMI_G1_GS) & SMI_G1_GS_PPUEN);
	}
	return gc;
}

static inline void smi_ppu_restore(struct mii_bus *smi_bus, uint16_t gc)
{
	if (gc & SMI_G1_GC_PPU)
	{
		smi_write(smi_bus, SMI_G1, SMI_G1_GC, gc);

		while((smi_read(smi_bus, SMI_G1, SMI_G1_GS) & SMI_G1_GS_PPUMODE) !=
		       (SMI_G1_GS_PPUEN | SMI_G1_GS_PPUDONE));
	}
}

int smi_phy_direct_read(struct mii_bus *bus, int addr, int reg)
{
	struct smi_bus *smi_phy = bus->priv;
	uint16_t gc;
	int ret;

	gc = smi_ppu_save_disable(smi_phy->mii);

	ret = smi_read(smi_phy->mii, addr, reg);

	smi_ppu_restore(smi_phy->mii, gc);
	return ret;
}

int smi_phy_direct_write(struct mii_bus *bus, int addr, int reg, uint16_t val)
{
	struct smi_bus *smi_phy = bus->priv;
	uint16_t gc;
	int ret;

	gc = smi_ppu_save_disable(smi_phy->mii);

	ret = smi_write(smi_phy->mii, addr, reg, val);

	smi_ppu_restore(smi_phy->mii, gc);
	return ret;
}

static int smi_phy_direct_bus_probe(struct device_d *dev)
{
	struct smi_bus_pdata *pdata = dev->platform_data;
	struct smi_bus *smi_phy;
	int ret;

	smi_phy = xzalloc(sizeof(*smi_phy));

	smi_phy->mii  = pdata->bus;

	smi_phy->bus.read   = smi_phy_direct_read;
	smi_phy->bus.write  = smi_phy_direct_write;
	smi_phy->bus.reset  = smi_reset;
	smi_phy->bus.priv   = smi_phy;
	smi_phy->bus.parent = dev;
	dev->priv = smi_phy;

	ret = mdiobus_register(&smi_phy->bus);
	if (ret)
		return ret;

	return 0;
}

static struct driver_d smi_phy_direct_bus_driver = {
	.name  = "mdio-smi-direct",
	.probe = smi_phy_direct_bus_probe,
};
register_driver_macro(coredevice, platform, smi_phy_direct_bus_driver);


static int smi_phy_wait_ready(struct mii_bus *smi_bus)
{
	int retries, sc;

	for (retries = 100; retries; retries--) {
		sc = smi_read(smi_bus, SMI_G2, SMI_G2_PC);
		if (!(sc & SMI_G2_PC_B))
			return 0;
	}
	return 1;
}

int smi_phy_indirect_read(struct mii_bus *bus, int addr, int reg)
{
	struct smi_bus *smi_phy = bus->priv;
	int ret = -EIO;

	if (smi_phy_wait_ready(smi_phy->mii))
		goto out;

	if (smi_write(smi_phy->mii, SMI_G2,
		      SMI_G2_PC, 0x9800 | (addr << 5) | reg))
		goto out;

	if (smi_phy_wait_ready(smi_phy->mii))
		goto out;

	ret = smi_read(smi_phy->mii, SMI_G2, SMI_G2_PD);
out:
	return ret;
}

int smi_phy_indirect_write(struct mii_bus *bus, int addr, int reg, uint16_t val)
{
	struct smi_bus *smi_phy = bus->priv;
	int ret = -EIO;

	if (smi_phy_wait_ready(smi_phy->mii))
		goto out;

	if (smi_write(smi_phy->mii, SMI_G2, SMI_G2_PD, val))
		goto out;

	if (smi_write(smi_phy->mii, SMI_G2,
		      SMI_G2_PC, 0x9400 | (addr << 5) | reg))
		goto out;

	if (smi_phy_wait_ready(smi_phy->mii))
		goto out;

	ret = 0;
out:
	return ret;
}

static int smi_phy_indirect_bus_probe(struct device_d *dev)
{
	struct smi_bus_pdata *pdata = dev->platform_data;
	struct smi_bus *smi_phy;
	int ret;

	smi_phy = xzalloc(sizeof(*smi_phy));

	smi_phy->mii  = pdata->bus;

	smi_phy->bus.read   = smi_phy_indirect_read;
	smi_phy->bus.write  = smi_phy_indirect_write;
	smi_phy->bus.reset  = smi_reset;
	smi_phy->bus.priv   = smi_phy;
	smi_phy->bus.parent = dev;
	dev->priv = smi_phy;

	ret = mdiobus_register(&smi_phy->bus);
	if (ret)
		return ret;

	return 0;
}

static struct driver_d smi_phy_indirect_bus_driver = {
	.name  = "mdio-smi-indirect",
	.probe = smi_phy_indirect_bus_probe,
};
register_driver_macro(coredevice, platform, smi_phy_indirect_bus_driver);
