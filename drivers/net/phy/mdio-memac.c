#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>

#include <linux/phy.h>

#include <mach/memac.h>

struct memac_mdio_regs {
	u32 cfg;
#define CFG_BSY			BIT(0)
#define CFG_RD_ER		BIT(1)
#define CFG_HOLD_SHIFT		2
#define CFG_PRE_DIS		BIT(5)
#define CFG_ENC45		BIT(6)
#define CFG_CLK_DIV_MASK	0x0000ff80
#define CFG_CLK_DIV_SHIFT	7
#define CFG_EHOLD		BIT(22)
#define CFG_NEG			BIT(23)
#define CFG_CIM			BIT(29)
#define CFG_CMP			BIT(30)
	u32 ctl;
#define CTL_DEV_ADDR_SHIFT	0
#define CTL_PORT_ADDR_SHIFT	5
#define CTL_POST_INC		BIT(14)
#define CTL_READ		BIT(15)
	u32 data;
	u32 addr;
} __packed;

struct memac_mdio {
	struct mii_bus bus;
	struct memac_mdio_pdata *pdata;
	struct memac_mdio_regs __iomem *regs;
};

static void __memac_mdio_set_freq(struct memac_mdio *mm, uint hz)
{
	uint div = mm->pdata->enet_freq / hz;
	uint clk_div = div / 2;
	u32 cfg;

	cfg = in_be32(&mm->regs->cfg);
	cfg &= ~CFG_CLK_DIV_MASK;
	cfg |= clk_div << CFG_CLK_DIV_SHIFT;
	out_be32(&mm->regs->cfg, cfg);
}

static int __memac_mdio_wait(struct memac_mdio *mm)
{
	uint64_t start = get_time_ns();

	while (!is_timeout(start, 10 * MSECOND)) {
		if (!(in_be32(&mm->regs->cfg) & CFG_BSY))
			return 0;
	}

	dev_err(&mm->bus.dev, "timeout\n");
	return -ETIMEDOUT;
}

static int memac_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct memac_mdio *mm = bus->priv;
	u32 ctl;
	int err;

	err = __memac_mdio_wait(mm);
	if (err)
		return err;

	ctl = CTL_READ | ((addr & 0x1f) << CTL_PORT_ADDR_SHIFT) | (reg & 0x1f);
	out_be32(&mm->regs->ctl, ctl);

	err = __memac_mdio_wait(mm);
	if (err)
		return err;

	if (in_be32(&mm->regs->cfg) & CFG_RD_ER)
		return -EIO;

	return in_be32(&mm->regs->data) & 0xffff;
}

static int memac_mdio_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct memac_mdio *mm = bus->priv;
	u32 ctl;
	int err;

	err = __memac_mdio_wait(mm);
	if (err)
		return err;

	ctl = ((addr & 0x1f) << CTL_PORT_ADDR_SHIFT) | (reg & 0x1f);
	out_be32(&mm->regs->ctl, ctl);

	out_be32(&mm->regs->data, val);
	return 0;
}

static int memac_mdio_reset(struct mii_bus *bus)
{
	return 0;
}
static void memac_mdio_remove(struct device_d *dev)
{
	struct mii_bus *bus = dev->priv;
	mdiobus_unregister(bus);
}
static int memac_mdio_probe(struct device_d *dev)
{
	struct memac_mdio *mm;
	struct memac_mdio_pdata *pdata;
	void *regs;

	pdata = dev->platform_data;
	if (!pdata || !pdata->enet_freq)
		return -EINVAL;

	regs = dev_get_mem_region(dev, 0);
	if (!regs)
		return -ENOMEM;

	mm = xzalloc(sizeof(*mm));
	mm->pdata = pdata;
	mm->regs = regs;

	mm->bus.parent = dev;
	mm->bus.priv   = mm;
	mm->bus.read   = memac_mdio_read;
	mm->bus.write  = memac_mdio_write;
	mm->bus.reset  = memac_mdio_reset;
	dev->priv = &mm->bus;

	/* allow boards to specify the MDC frequency, fall-back to
	 * standard's max, 2.5 MHz */
	__memac_mdio_set_freq(mm, pdata->mdc_freq? : 2500000);

	/* only clause 22 for now */
	clrbits_be32(&mm->regs->cfg, CFG_ENC45);

	return mdiobus_register(&mm->bus);
}

static struct driver_d memac_mdio_driver = {
	.name  = "memac-mdio",
	.probe = memac_mdio_probe,
        .remove = memac_mdio_remove,
};
register_driver_macro(coredevice, platform, memac_mdio_driver);
