/*
 * Marvell xCAT3 Packet Processor
 *
 * Copyright (C) 2014 Westermo Teleindustri AB
 *
 * Author: Anders Öhlander <anders.ohlander@westermo.se>
 *         Joacim Zetterling <joacim.zetterling@westermo.se>
 *         Tobias Waldekranz <tobias@waldekranz.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "xc3_private.h"

typedef enum {
	SERDES_SPEED_1_25G = 0,
	SERDES_SPEED_5G,

	__SERDES_SPEED_MAX
} xc3_serdes_speed_t;

typedef struct {
	u16  reg;
	u16  mask;
	u16  data[__SERDES_SPEED_MAX];
	int  delay_ms;
} xc3_serdes_op_t;

#define XC3_SERDES_OP_TERM { .delay_ms = -1 }

static const xc3_serdes_op_t sgmii_reset[] = {
	{ .reg =   0x0, .data = { 0x4000,    0x0 }},	/* SERDES_SD_RESET_SEQ Sequence init */
	{ .reg =   0x4, .data = { 0x8809, 0x8809 }},	/* SERDES_SD_RESET_SEQ Sequence init */
	{ .reg =   0x4, .data = {    0x0,    0x0 }, .mask = 0xffbf },	/* SERDES_RF_RESET Sequence init */
	{ .reg =   0x8, .data = {    0x0,    0x0 }},	/* SERDES_SYNCE_RESET_SEQ Sequence init */
	{ .reg =   0x4, .data = {   0x20,   0x20 }, .mask = 0xffdf },	/* SERDES_CORE_RESET_SEQ Sequence init */
	{ .reg = 0x93c, 0x400 , {    0x0,    0x0 }, .mask = 0xfbff },	/* SERDES_CORE_UNRESET_SEQ Sequence init */

	XC3_SERDES_OP_TERM
};

static const xc3_serdes_op_t serdes_ext_speed_config[] = {
	{ .reg =   0x0, .data = { 0x4330,  0x198 }},	/* Setting PIN_GEN_TX, PIN_GEN_RX */	
	{ .reg =  0x28, .data = {    0xc,    0xc }},	/* PIN_FREF_SEL=C (156.25MHz) */

	XC3_SERDES_OP_TERM
};

static const xc3_serdes_op_t serdes_int_speed_config[] = {
	{ .reg = 0x804, .data = { 0xfd8c, 0xfd8c }},	/*  */
	{ .reg = 0x818, .data = { 0x4f00, 0x4f00 }},	/*  squelch in */
	{ .reg = 0x81C, .data = { 0xf047, 0xf047 }},	/*  DFE UPDAE all coefficient DFE_RES = 3.3mv */
	{ .reg = 0x834, .data = { 0x406c, 0x406c }},	/*  TX_AMP=22, AMP_ADJ=0 */
	{ .reg = 0x838, .data = { 0x1e40, 0x1e40 }},	/*  MUPI/F=2, rx_digclkdiv=3 */
	{ .reg = 0x894, .data = { 0x1fff, 0x1fff }},	/*  */
	{ .reg = 0x898, .data = {   0x66,   0x33 }},	/*  set password */
	{ .reg = 0x904, .data = { 0x2208, 0x2208 }},	/*  */
	{ .reg = 0x908, .data = { 0x243f, 0x243f }},	/*  continues vdd_calibration */
	{ .reg = 0x914, .data = { 0x46cf, 0x46cf }},	/*  EMPH0 enable, EMPH_mode=2 */
	{ .reg = 0x934, .data = {   0x4a,   0x4a }},	/*  RX_IMP_VTH=2, TX_IMP_VTH=2 */
	{ .reg = 0x93C, .data = { 0xe028, 0xe028 }},	/*  Force ICP=8 */
	{ .reg = 0x940, .data = {  0x800,  0x800 }},	/*  clk 8T enable for 10G and up */
	{ .reg = 0x954, .data = {   0x87,   0x87 }},	/*  rxdigclk_div_force=1 */
	{ .reg = 0x968, .data = { 0xe014, 0xe014 }},	/*  */
	{ .reg = 0x96C, .data = {   0x14,   0x14 }},	/*  DTL_FLOOP_EN=0 */
	{ .reg = 0x984, .data = { 0x10f3, 0x10f3 }},	/*  */
	{ .reg = 0x9A8, .data = { 0x4000, 0x4000 }},	/*  */
	{ .reg = 0x9AC, .data = { 0x8498, 0x8498 }},	/*  */
	{ .reg = 0x9DC, .data = {  0x780,  0x780 }},	/*  */
	{ .reg = 0x9E0, .data = {  0x3fe,  0x3fe }},	/*  PHY_MODE=0x4,FREF_SEL=0xC */
	{ .reg = 0xa14, .data = { 0x4418, 0x4418 }},	/*  PHY_MODE=0x4,FREF_SEL=0xC */
	{ .reg = 0xa20, .data = {  0x400,  0x400 }},	/*  */
	{ .reg = 0xa28, .data = { 0x2fc0, 0x2fc0 }},	/*  */
	{ .reg = 0xa68, .data = { 0x8c02, 0x8c02 }},	/*  */
	{ .reg = 0xa78, .data = { 0x21f3, 0x21f3 }},	/*  */
	{ .reg = 0xa80, .data = { 0xc9f8, 0xc9f8 }},	/*  */
	{ .reg = 0xa9C, .data = {  0x5bc,  0x5bc }},	/*  */
	{ .reg = 0xaDC, .data = { 0x2233, 0x2233 }},	/*  */
	{ .reg = 0xb1C, .data = {  0x318,  0x318 }},	/*  */
	{ .reg = 0xb30, .data = {  0x13f,  0x12f }},	/*  */
	{ .reg = 0xb34, .data = {  0xc03,  0xc03 }},	/*  */
	{ .reg = 0xb38, .data = { 0x3c00, 0x3c00 }},	/*  */
	{ .reg = 0xb3C, .data = { 0x3c00, 0x3c00 }},	/*  */
	{ .reg = 0xb68, .data = { 0x1000, 0x1000 }},	/*  */
	{ .reg = 0xb6C, .data = {  0xad9,  0xad9 }},	/*  */
	{ .reg = 0xb78, .data = { 0x1800, 0x1800 }},	/*  */
	{ .reg = 0xc18, .data = { 0xe737, 0xe737 }},	/*  */
	{ .reg = 0xc20, .data = { 0x9ce0, 0x9ce0 }},	/*  */
	{ .reg = 0xc40, .data = {   0x3e, 0x503e }},	/*  */
	{ .reg = 0xc44, .data = { 0x2681, 0x2561 }},	/*  */
	{ .reg = 0xc68, .data = {    0x1,    0x1 }},	/*  */
	{ .reg = 0xc6C, .data = { 0xfc7c, 0xfc7c }},	/*  */
	{ .reg = 0xcA0, .data = {  0x104,  0x104 }},	/*  */
	{ .reg = 0xcA4, .data = {  0x302,  0x302 }},	/*  */
	{ .reg = 0xcA8, .data = {  0x202,  0x202 }},	/*  */
	{ .reg = 0xcAC, .data = {  0x202,  0x202 }},	/*  */
	{ .reg = 0x90C, .data = {  0x830,  0x830 }},	/*  txclk regulator threshold set */
	{ .reg = 0x910, .data = {  0xf30,  0xf30 }},	/*  txdata regulator threshold set */
	{ .reg = 0xc24, .data = {    0x5,    0x5 }},	/*  os_ph_step_size[1:0]= 0 */
	{ .reg = 0xb9C, .data = {  0x559,  0x559 }},	/*  */
	{ .reg = 0xa44, .data = { 0x3f3e, 0x3f3e }},	/*  FFE table RC= 0F/1F */
	{ .reg = 0xa48, .data = { 0x4f4e, 0x4f4e }},	/*  FFE table RC= 2F/3F */
	{ .reg = 0xa4C, .data = { 0x5f5e, 0x5f5e }},	/*  FFE table RC= 4F/0E */
	{ .reg = 0xa50, .data = { 0x6f6e, 0x6f6e }},	/*  FFE table RC= 1E/2E */
	{ .reg = 0xa54, .data = { 0x3d3c, 0x3d3c }},	/*  FFE table RC= 3E/4E */
	{ .reg = 0xa58, .data = { 0x4d4c, 0x4d4c }},	/*  FFE table RC= 0D/1D */
	{ .reg = 0xa5C, .data = { 0x5d5c, 0x5d5c }},	/*  FFE table RC= 2D/3D */
 	{ .reg = 0xa60, .data = { 0x2f2e, 0x2f2e }},	/*  FFE table RC= 4D/0C */
	{ .reg = 0xa2C, .data = { 0x4070, 0x4070 }},	/*  select ffe table mode */
	{ .reg = 0x9CC, .data = {  0x451,  0x451 }},	/*  */
	{ .reg = 0x970, .data = { 0xdd8e, 0xdd8e }},	/*  */
	{ .reg = 0xc1C, .data = { 0x303f, 0x303f }},	/*  */

	XC3_SERDES_OP_TERM
};

static const xc3_serdes_op_t sgmii_power_up[] =
{
	{ .reg =   0x0, .data = { 0x5b32, 0x199a }},
	{ .reg =   0x8, .data = {   0x10,   0x10 }},
	{ .reg =   0x8, .data = { 0x8032, 0x8032 }, .delay_ms = 8 },
	{ .reg =   0x8, .data = { 0x6432, 0x6432 }, .delay_ms = 8 },
	{ .reg = 0x95c, .data = {    0x5,    0x5 }, .delay_ms = 1 },
	{ .reg = 0x95c, .data = {    0x1,    0x1 }, .delay_ms = 1 },

	XC3_SERDES_OP_TERM
};

static const xc3_serdes_op_t sgmii_unreset[] = {
	{ .reg =   0x4, .data = {    0x8,    0x8 }, .mask = 0xfff7 },	/* SERDES_SD_UNRESET_SEQ Sequence init */
	{ .reg =   0x4, .data = {   0x40,   0x40 }, .mask = 0xffff },	/* SERDES_RF_UNRESET Sequence init */
	{ .reg =   0x4, .data = { 0xdd69, 0xdd69 }, .mask = 0x00ff },	/* SERDES_SYNCE_UNRESET_SEQ Sequence init */
	{ .reg =   0x8, .data = {    0xB,    0xB }, .mask = 0xfff4 },	/* SERDES_SYNCE_UNRESET_SEQ Sequence init */
	{ .reg =   0x4, .data = { 0xdd69, 0xdd69 }, .mask = 0x00ff },	/* SERDES_SYNCE_UNRESET_SEQ Sequence init */

	XC3_SERDES_OP_TERM
};

static const xc3_serdes_op_t *serdes_init_seqs[] = {
	sgmii_reset,
	serdes_ext_speed_config,
	serdes_int_speed_config,
	sgmii_power_up,
	sgmii_unreset,

	NULL
};

static void xc3_serdes_op_one(u32 __iomem *regs, const xc3_serdes_op_t *op,
			      xc3_serdes_speed_t speed)
{
	u32 __iomem *reg = regs + (op->reg >> 2);
	u32 out = op->data[speed];

	if (op->mask)
		out |= readl(reg) & op->mask;

	writel(out, reg);

	if (op->delay_ms)
		mdelay(op->delay_ms);
}

static void xc3_serdes_op_seq(u32 __iomem *regs, const xc3_serdes_op_t *ops,
			      xc3_serdes_speed_t speed)
{
	for (; ops->delay_ms >= 0; ops++)
		xc3_serdes_op_one(regs, ops, speed);
}

#define SERDES_BASE		0x13000000

#define SERDES_PORT_STRIDE	0x1000
#define SERDES_PORT(_i)		(SERDES_BASE + (SERDES_PORT_STRIDE * (_i)))

typedef struct {
	u32 addr;
	xc3_serdes_speed_t speed;
} xc3_serdes_t;

static const xc3_serdes_t serdes_ports[] = {
	{ .addr = SERDES_PORT(0), .speed = SERDES_SPEED_5G },
	{ .addr = SERDES_PORT(1), .speed = SERDES_SPEED_5G },
	{ .addr = SERDES_PORT(2), .speed = SERDES_SPEED_5G },
	{ .addr = SERDES_PORT(3), .speed = SERDES_SPEED_5G },
	{ .addr = SERDES_PORT(4), .speed = SERDES_SPEED_5G },
	{ .addr = SERDES_PORT(5), .speed = SERDES_SPEED_5G },

	{ .addr = SERDES_PORT(6), .speed = SERDES_SPEED_1_25G },
	{ .addr = SERDES_PORT(7), .speed = SERDES_SPEED_1_25G },
	{ .addr = SERDES_PORT(8), .speed = SERDES_SPEED_1_25G },
	{ .addr = SERDES_PORT(9), .speed = SERDES_SPEED_1_25G },

	{ .addr = 0 }
};

static void xc3_serdes_setup(struct xc3 *xc3, const xc3_serdes_t *serdes)
{
	const xc3_serdes_op_t **seqs;
	
	for (seqs = serdes_init_seqs; *seqs; seqs++) {
		u32 __iomem *regs = xc3_win_get(xc3, serdes->addr);

		xc3_serdes_op_seq(regs, *seqs, serdes->speed);

		xc3_win_put(xc3);
	}
}

static int xc3_serdes_probe(struct device_d *pdev)
{
	struct xc3 *xc3 = pdev->parent->type_data;
	const xc3_serdes_t *serdes;

	for (serdes = serdes_ports; serdes->addr; serdes++)
		xc3_serdes_setup(xc3, serdes);

	pr_info("%s: probed\n", dev_name(pdev));
	return 0;
}

static struct of_device_id xc3_serdes_dt_ids[] = {
	{ .compatible = "marvell,xcat-3-serdes" },
	{ }
};

static struct driver_d xc3_serdes_driver = {
	.name   = "xc3_serdes",
	.probe  = xc3_serdes_probe,
	.of_compatible = DRV_OF_COMPAT(xc3_serdes_dt_ids),
};
device_platform_driver(xc3_serdes_driver);
