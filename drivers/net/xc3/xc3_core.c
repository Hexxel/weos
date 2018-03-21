/*
 * Marvell xCAT 3 Packet Processor
 *
 * Copyright (C) 2014 Westermo Teleindustri AB
 *
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 *         Joacim Zetterling <joacim.zetterling@westermo.se>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "xc3_private.h"

#define DFX_RESET_CONTROL_DEV_EN BIT(0)

struct xc3_mg {
	u32  resvd0x0[0x4c >> 2];
	u32 device_id;
	u32 vendor_id;
	u32  resvd0x54;
	u32 global_ctrl;
#define MG_GLOBAL_CTRL_SDMA_PSAVE BIT(19)
	u32  resvd0x5c[(0x120 - 0x5c) >> 2];
	u32 addr_compl[8];
	u32 legacy_addr_compl;
#define MG_LEGACY_ADDR_COMPL (1 << 16)
	u32  resvd0x144[(0x20c - 0x144) >> 2];
	struct {
		u32 ba;
		u32 s;
	} __packed bar[6];
	u32 bar_hi_remap[6];
	u32 bar_ctrl[6];
#define BAR_CTRL_WIN_AP_RW (3 << 1)
#define BAR_CTRL_DISABLE   BIT(0)
} __packed;

u32 xc3_device_id(struct xc3 *xc3)
{
	return readl(&xc3->mg->device_id);
}

void xc3_tbl_read(struct xc3 *xc3, u32 base, u32 idx, struct xc3_row *row)
{
	u32 sz = row->size;
	u32 *src = xc3_win_get(xc3, base + idx * sz);
	int i;

	for (i = 0; i < (sz >> 2); i++)
		row->data[i] = readl(&src[i]);

	xc3_win_put(xc3);
}

void xc3_tbl_write(struct xc3 *xc3, u32 base, u32 idx, const struct xc3_row *row)
{
	u32 sz = row->size;
	u32 *dst = xc3_win_get(xc3, base + idx * sz);
	int i;

	for (i = 0; i < (sz >> 2); i++)
		writel(row->data[i], &dst[i]);

	xc3_win_put(xc3);
}

static int xc3_addr_compl_setup_win(struct xc3 *xc3, u32 base, u32 remap)
{
	int window = remap >> 19;

	/* window 0 is fixed at base 0 */
	if (window == 0)
		return base ? -EINVAL : 0;

	if (window > 7)
		return -EINVAL;

	writel(base >> 19, &xc3->mg->addr_compl[window]);
	writel(base >> 19, &xc3->mg->addr_compl[window]); /* dummy write for correct op. (JZE) */
	return 0;
}

void __iomem *xc3_win_get(struct xc3 *xc3, u32 base)
{
	mutex_lock(&xc3->dyn_win_lock);

	if (xc3_addr_compl_setup_win(xc3, base, DYN_WIN))
		return NULL;

	return (char __iomem *)xc3->dyn_win + (base & (ADDR_COMPL_SZ - 1));
}

void xc3_win_put(struct xc3 *xc3)
{
	mutex_unlock(&xc3->dyn_win_lock);
}

void xc3_rmw(struct xc3 *xc3, u32 address, u32 mask, u32 val)
{
	u32 __iomem *reg = xc3_win_get(xc3, address);

	writel((readl(reg) & mask) | val, reg);

	xc3_win_put(xc3);
}

void xc3_reg_read(struct xc3 *xc3, u32 reg, u32* val)
{
	u32 *src = xc3_win_get(xc3, reg);
	*val = readl(&src);
	xc3_win_put(xc3);
}

void xc3_reg_write(struct xc3 *xc3, u32 reg, u32 val)
{
	u32 *dst = xc3_win_get(xc3, reg);
	writel(val, &dst);
	xc3_win_put(xc3);
}

static int xc3_addr_compl_probe(struct xc3 *xc3)
{
	struct device_node *np = xc3->pdev->device_node;
	struct property *prop;
	const __be32 *p;
	u32 legacy, val, base = 0;
	int err = 0, i = 0;

	legacy = readl(&xc3->mg->legacy_addr_compl);
	if (legacy & MG_LEGACY_ADDR_COMPL)
		writel(legacy & ~MG_LEGACY_ADDR_COMPL,
		       &xc3->mg->legacy_addr_compl);

	of_property_for_each_u32(np, "ranges", prop, p, val) {
		if (i % XC3_RANGES_WIDTH == 0) {
			base = val;
		} else if (i % XC3_RANGES_WIDTH == 2) {
			err = xc3_addr_compl_setup_win(xc3, base, val);
			if (err)
				break;
		}
		i++;
	}

	return err;
}

static int xc3_bar_probe(struct xc3 *xc3)
{
	struct device_node *mem = of_find_node_by_name(NULL, "memory");
	u32 base/* , attr = 0xd, target = 0x0 */;
	const __be32 *basep;
	u64 size;

	if (!mem)
		return -ENODEV;

	basep = of_get_address(mem, 0, &size, NULL);
	if (!basep)
		return -EINVAL;

	base = of_read_number(basep, 1);

	//xc3_info(xc3, "mapping %lluM at %#x\n", size >> 20, base);
	pr_info("mapping %lluM at %#x\n", size >> 20, base);

	/* writel(base | (attr << 8) | target, &xc3->mg->bar[0].ba); */
	/* writel((size - 1) & 0xffff0000, &xc3->mg->bar[0].s); */

	/* WKZ DEBUG */
	/* attr=0x10 might point to the wrong CS here. thanks for
	 * nothing, marvell docs */
	writel(0x00000000 | (0x10 << 8) | 0, &xc3->mg->bar[0].ba);
	writel(((256 << 20) - 1) & 0xffff0000, &xc3->mg->bar[0].s);
	writel(BAR_CTRL_WIN_AP_RW, &xc3->mg->bar_ctrl[0]);

	/* attr=0x10 stolen from appDemo startup */
	writel(0x10000000 | (0x10 << 8) | 0, &xc3->mg->bar[1].ba);
	writel(((256 << 20) - 1) & 0xffff0000, &xc3->mg->bar[1].s);
	writel(BAR_CTRL_WIN_AP_RW, &xc3->mg->bar_ctrl[1]);

	return 0;
}

static int xc3_probe(struct device_d *pdev)
{
	struct xc3 *xc3;
	struct device_node *controller;
	struct device_node *np = pdev->device_node;
	const __be32 *prop;
	u32 __iomem *dfx_reset_control;
	int err;
	u32 *br_gbl_conf1;

	dfx_reset_control = of_iomap(np, 0);
	if (!dfx_reset_control) {
		pr_err("error: of_iomap: %d\n", -EINVAL);
		return -EINVAL;
	}

	xc3 = xzalloc(sizeof(*xc3));
	if (!xc3)
		return -ENOMEM;

	xc3->pdev = pdev;
	pdev->type_data = xc3;

	prop = of_get_property(np, "controller", NULL);
	if (!prop) {
		pr_err("error: of_iomap: %d\n", -EINVAL);
		return -EINVAL;
	}

	controller = of_find_node_by_phandle(be32_to_cpup(prop));
	if (!controller) {
		pr_err("error: of_find_node_by_phandle: %d\n", -ENODEV);
		return -ENODEV;
	}

	xc3->mg = of_iomap(controller, 0);
	if (!xc3->mg) {
		pr_err("error: of_iomap: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	mutex_init(&xc3->dyn_win_lock);
	xc3->dyn_win = of_iomap(controller, 2);
	if (!xc3->dyn_win) {
		pr_err("error: mutex_init: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	err = xc3_addr_compl_probe(xc3);
	if (err) {
		pr_err("error: xc3_addr_compl_probe: %d\n", err);
		return err;
	}

	err = xc3_bar_probe(xc3);
	if (err) {
		pr_err("error: xc3_bar_probe: %d\n", err);
		return err;
	}

// 	xc3_br_init(xc3);

	err = of_platform_populate(np, NULL, pdev);
	if (err < 0)
		return err;

	err = xc3_cdev_init(xc3);
	if (err)
//		xc3_err(xc3, "error: cdev_init: %d\n", err);
		pr_info("error: cdev_init: %d\n", err);

// 	err = xc3_debugfs_init(xc3);
// 	if (err)
// 		xc3_err(xc3, "error: debugfs_init: %d\n", err);

	writel(readl(dfx_reset_control) | DFX_RESET_CONTROL_DEV_EN,
	       dfx_reset_control);

	/* Disable the bridge DSA source filter */
	br_gbl_conf1 = xc3_win_get(xc3, 0x01040004);
	writel(readl(br_gbl_conf1) | (1 << 31), br_gbl_conf1);
	xc3_win_put(xc3);

	pr_info("%s: probed\n", dev_name(pdev));

	return 0;
}

static const struct of_device_id xc3_match[] = {
	{ .compatible = "marvell,xcat-3-pp" },
	{ }
};

static struct of_device_id xc3_dt_ids[] = {
	{ .compatible = "marvell,xcat-3-pp" },
	{ }
};

static struct driver_d xc3_driver = {
	.name   = "xc3",
	.probe  = xc3_probe,
	.of_compatible = DRV_OF_COMPAT(xc3_dt_ids),
};

device_platform_driver(xc3_driver);
