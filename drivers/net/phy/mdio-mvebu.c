/*
 * Marvell MVEBU SoC MDIO interface driver
 *
 * (C) Copyright 2014
 *   Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *   Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * based on mvmdio driver from Linux
 *   Copyright (C) 2012 Marvell
 *     Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * Since the MDIO interface of Marvell network interfaces is shared
 * between all network interfaces, having a single driver allows to
 * handle concurrent accesses properly (you may have four Ethernet
 * ports, but they in fact share the same SMI interface to access
 * the MDIO bus). This driver is currently used by the mvneta and
 * mv643xx_eth drivers.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/phy.h>

#include <of.h>
#include <of_address.h>
#include <of_net.h>


#define SMI_DATA_SHIFT          		0
#define SMI_PHY_ADDR_SHIFT      		16
#define SMI_PHY_REG_SHIFT       		21
#define SMI_READ_OPERATION      		BIT(26)
#define SMI_WRITE_OPERATION     		0
#define SMI_READ_VALID          		BIT(27)
#define SMI_BUSY                		BIT(28)

#define XSMI_DATA_SHIFT          	0
#define XSMI_PAGE_SHIFT					5
#define XSMI_PHY_ADDR_SHIFT      	16
#define XSMI_PHY_REG_SHIFT       	21
#define XSMI_WRITE_OPERATION     	BIT(26)
#define XSMI_READ_INCR_OPERATION		BIT(27)
#define XSMI_READ_ONLY_OPERATION		BIT(26) | BIT(27)
#define XSMI_ADDRESS_THEN_WRITE  	BIT(26) | BIT(28)
#define XSMI_ADDRESS_THEN_INCR_READ	BIT(27) | BIT(28)
#define XSMI_ADDRESS_THEN_READ	  	BIT(26) | BIT(27) | BIT(28)
#define XSMI_READ_VALID          	BIT(29)
#define XSMI_BUSY                	BIT(30)

#define ERR_INT_CAUSE		0x007C
#define  ERR_INT_SMI_DONE	BIT(4)
#define ERR_INT_MASK		BIT(7)

#define MVMDIO_XC3_LMSMISC_INVERT_MDC   BIT(20)


struct mdio_priv {
	struct mii_bus miibus;
	void __iomem *regs;
	struct clk *clk;
	
	int is_c45;
	int page;
};

#define SMI_POLL_TIMEOUT	(10 * MSECOND)
#define XSMI_POLL_TIMEOUT	(10 * MSECOND)


static int mvebu_mdio_wait_ready(struct mdio_priv *priv)
{
	int ret;

	if (priv->is_c45)
		ret = wait_on_timeout(XSMI_POLL_TIMEOUT, (readl(priv->regs) & XSMI_BUSY) == 0);
	else
		ret = wait_on_timeout(SMI_POLL_TIMEOUT, (readl(priv->regs) & SMI_BUSY) == 0);
	if (ret)
		dev_err(&priv->miibus.dev, "timeout, SMI/XSMI busy for too long\n");

	return ret;
}

static int mvebu_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct mdio_priv *priv = bus->priv;
	u32 val;
	int ret;

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		goto out;

	if (priv->is_c45) {
		writel((addr << XSMI_PHY_ADDR_SHIFT) | (3 << XSMI_PHY_REG_SHIFT) | 
			(priv->page << XSMI_PAGE_SHIFT) | (reg << XSMI_DATA_SHIFT) | 
			0x8000, priv->regs);

		ret = mvebu_mdio_wait_ready(priv);
		if (ret)
			goto out;

		writel((XSMI_READ_ONLY_OPERATION | (addr << XSMI_PHY_ADDR_SHIFT) | 
			(3 << XSMI_PHY_REG_SHIFT)), priv->regs);
	}
	else {
		writel(((addr << SMI_PHY_ADDR_SHIFT) | (reg << SMI_PHY_REG_SHIFT) |
			SMI_READ_OPERATION),	priv->regs);
	}

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		goto out;

	val = readl(priv->regs);
	if (priv->is_c45)
		ret = val & XSMI_READ_VALID;
	else
		ret = val & SMI_READ_VALID;
	if (!ret) {
		dev_err(&priv->miibus.dev, "SMI/XSMI bus read not valid\n");
		ret = -ENODEV;
		goto out;
	}

	val = readl(priv->regs);
	ret = val & 0xFFFF;
out:
	return ret;
}

static int mvebu_mdio_write(struct mii_bus *bus, int addr, int reg, u16 data)
{
	struct mdio_priv *priv = bus->priv;
	int ret;

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		goto out;

	if (priv->is_c45) {
		if (reg == 0x16)
			priv->page = data;
		writel(((addr << XSMI_PHY_ADDR_SHIFT) | (3 << XSMI_PHY_REG_SHIFT) | 
			(priv->page << XSMI_PAGE_SHIFT) | (reg << XSMI_DATA_SHIFT) |
			0x8000), priv->regs);

		ret = mvebu_mdio_wait_ready(priv);
		if (ret)
			goto out;

		writel(((addr << XSMI_PHY_ADDR_SHIFT) | (3 << XSMI_PHY_REG_SHIFT) | 
			(data << XSMI_DATA_SHIFT) | XSMI_WRITE_OPERATION), priv->regs);
	}
	else {
		writel(((addr << SMI_PHY_ADDR_SHIFT) | (reg << SMI_PHY_REG_SHIFT)  |
			SMI_WRITE_OPERATION | (data << SMI_DATA_SHIFT)), priv->regs);
	}

out:
	return ret;
}

static int xc3_mdio_probe(struct device_d *dev)
{
	u32 __iomem *lmsmisc;
	struct device_node *np = dev->device_node;

   if (!np) {
      pr_err("[xc3_mdio_probe] error %d (line %d)\n", -ENOMEM, __LINE__);
      return -ENOMEM;
   }

	lmsmisc = of_iomap(np, 1);
	if (!lmsmisc) {
		pr_err("[xc3_mdio_probe] error %d (line %d)\n", -ENOMEM, __LINE__);
		return -ENOMEM;
	}

	/* Clock polarity is inverted by default, causing the sampled
	 * data to be left-shifted-by-one. */
	writel(readl(lmsmisc) & ~MVMDIO_XC3_LMSMISC_INVERT_MDC, lmsmisc);

	return 0;
}

static int mvebu_mdio_probe(struct device_d *dev)
{
	struct mdio_priv *priv;
	int ret;
	int is_c45 = 0;

	priv = xzalloc(sizeof(*priv));
	dev->priv = priv;
	
 	if ((dev->device_node) && 
		 (of_device_is_compatible(dev->device_node, "marvell,xcat-3-mdio"))) {
			ret = xc3_mdio_probe(dev);
			if (ret)
				return ret;
 	}

 	if ((dev->device_node) && 
		 (of_device_is_compatible(dev->device_node, "marvell,xcat-3-xsmi-mdio"))) {
		is_c45 = 1;
	}
	
	priv->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);
	clk_enable(priv->clk);

	priv->is_c45 = is_c45;
	priv->page = 0;
	priv->miibus.dev.device_node = dev->device_node;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;
	priv->miibus.read = mvebu_mdio_read;
	priv->miibus.write = mvebu_mdio_write;

	ret = mdiobus_register(&priv->miibus);
	return ret;
}

static void mvebu_mdio_remove(struct device_d *dev)
{
	struct mdio_priv *priv = dev->priv;

	mdiobus_unregister(&priv->miibus);

	clk_disable(priv->clk);
}

static struct of_device_id mvebu_mdio_dt_ids[] = {
	{ .compatible = "marvell,orion-mdio" },
	{ .compatible = "marvell,xcat-3-mdio" },
	{ .compatible = "marvell,xcat-3-xsmi-mdio" },
	{ }
};

static struct driver_d mvebu_mdio_driver = {
	.name   = "mvebu-mdio",
	.probe  = mvebu_mdio_probe,
	.remove = mvebu_mdio_remove,
	.of_compatible = DRV_OF_COMPAT(mvebu_mdio_dt_ids),
};
device_platform_driver(mvebu_mdio_driver);