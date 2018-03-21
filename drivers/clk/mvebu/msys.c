/*
 * Marvell MSys SoC clocks
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux Marvell MVEBU clock providers
 *   Copyright (C) 2012 Marvell
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <io.h>

#include "common.h"

/*
 * Core Clocks
 */
#define SARL										0	/* Low part [0:31] */
#define   SARL_MSYS_SAR_STAT_0				(SARL + 0x00)
#define   SARL_MSYS_SAR_STAT_1				(SARL + 0x04)
#define	 SARL_MSYS_PCLK_FREQ_OPT			18
#define	 SARL_MSYS_PCLK_FREQ_OPT_MASK		0x7
#define	 SARL_MSYS_FAB_FREQ_OPT				21
#define	 SARL_MSYS_FAB_FREQ_OPT_MASK		0x7

enum { MSYS_CPU_TO_NBCLK, MSYS_CPU_TO_HCLK, MSYS_CPU_TO_DRAMCLK };

static const struct coreclk_ratio msys_coreclk_ratios[] = {
	{ .id = MSYS_CPU_TO_NBCLK, .name = "nbclk" },
	{ .id = MSYS_CPU_TO_HCLK, .name = "hclk" },
	{ .id = MSYS_CPU_TO_DRAMCLK, .name = "dramclk" },
};

static const u32 msys_tclk_freqs[] = {
	290000000,
	250000000,
	222000000,
	167000000,
	200000000,
	133000000,
	360000000,
	0
};

static u32 msys_get_tclk_freq(void __iomem *sar)
{
#if 0
	u32 tclk_freq;
	u8 tclk_freq_select = 0;

	tclk_freq_select = ((readl(sar + SARL_MSYS_SAR_STAT_1) >> SARL_MSYS_FAB_FREQ_OPT) &
			   SARL_MSYS_FAB_FREQ_OPT_MASK);
	if (tclk_freq_select >= ARRAY_SIZE(msys_tclk_freqs)) {
		pr_err("CPU freq select unsupported %d\n", tclk_freq_select);
		tclk_freq = 0;
	} else
		tclk_freq = msys_tclk_freqs[tclk_freq_select];

	// pr_info("regs = 0x%.8x, val = 0x%.8x\n", (unsigned int)sar + SARL_MSYS_SAR_STAT_1, readl(sar + SARL_MSYS_SAR_STAT_1));
	pr_info("CORE (TCKL) freq set to %d Hz (sel=%d)\n", tclk_freq, tclk_freq_select);
	return tclk_freq;
#endif
   /* MSYS TCLK frequency is fixed to 200MHz (???) */
   return 200000000;
}

static const u32 msys_cpu_freqs[] = {
	400000000,
	0,
	667000000,
	800000000,
	0,
	800000000,
	0,
	0
};

static u32 msys_get_cpu_freq(void __iomem *sar)
{
#if 0
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar + SARL_MSYS_SAR_STAT_1) >> SARL_MSYS_PCLK_FREQ_OPT) &
			   SARL_MSYS_PCLK_FREQ_OPT_MASK);
	if (cpu_freq_select >= ARRAY_SIZE(msys_cpu_freqs)) {
		pr_err("CPU freq select unsupported %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = msys_cpu_freqs[cpu_freq_select];

	// pr_info("regs = 0x%.8x, val = 0x%.8x\n", (unsigned int)sar + SARL_MSYS_SAR_STAT_1, readl(sar + SARL_MSYS_SAR_STAT_1));
	pr_info("CPU freq set to %d Hz (sel=%d)\n", cpu_freq, cpu_freq_select);
	return cpu_freq;
#endif
   return 800000000;
}

static const int msys_nbclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 2}, {2, 2},
	{1, 2}, {1, 2}, {1, 1}, {2, 3},
	{0, 1}, {1, 2}, {2, 4}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {2, 2},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int msys_hclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 6}, {2, 3},
	{1, 3}, {1, 4}, {1, 2}, {2, 6},
	{0, 1}, {1, 6}, {2, 10}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 2},
	{2, 6}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int msys_dramclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 3}, {2, 3},
	{1, 3}, {1, 2}, {1, 2}, {2, 6},
	{0, 1}, {1, 3}, {2, 5}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void msys_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar + SARL_MSYS_SAR_STAT_1) >> SARL_MSYS_FAB_FREQ_OPT) &
		SARL_MSYS_FAB_FREQ_OPT_MASK);

	switch (id) {
	case MSYS_CPU_TO_NBCLK:
		pr_info("CPU (NBCLK) clock ratios id = %d, mult = %d, div = %d.\n", id, (int)msys_nbclk_ratios[opt][0], (int)msys_nbclk_ratios[opt][1]);
		*mult = msys_nbclk_ratios[opt][0];
		*div = msys_nbclk_ratios[opt][1];
		break;
	case MSYS_CPU_TO_HCLK:
		pr_info("CPU (HCLK) clock ratios id = %d, mult = %d, div = %d.\n", id, (int)msys_hclk_ratios[opt][0], (int)msys_hclk_ratios[opt][1]);
		*mult = msys_hclk_ratios[opt][0];
		*div = msys_hclk_ratios[opt][1];
		break;
	case MSYS_CPU_TO_DRAMCLK:
		pr_info("CPU (DRAMCLK) clock ratios id = %d, mult = %d, div = %d.\n", id, (int)msys_dramclk_ratios[opt][0], (int)msys_dramclk_ratios[opt][1]);
		*mult = msys_dramclk_ratios[opt][0];
		*div = msys_dramclk_ratios[opt][1];
		break;
	}
}

const struct coreclk_soc_desc msys_coreclks = {
	.get_tclk_freq = msys_get_tclk_freq,
	.get_cpu_freq = msys_get_cpu_freq,
	.get_clk_ratio = msys_get_clk_ratio,
	.ratios = msys_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(msys_coreclk_ratios),
};

/*
 * Clock Gating Control
 */
const struct clk_gating_soc_desc msys_gating_desc[] = {
	{ "audio", NULL, 0 },
	{ "ge3", NULL, 1 },
	{ "ge2", NULL,  2 },
	{ "ge1", NULL, 3 },
	{ "ge0", NULL, 4 },
	{ "pex00", NULL, 5 },
	{ "pex01", NULL, 6 },
	{ "pex02", NULL, 7 },
	{ "pex03", NULL, 8 },
	{ "pex10", NULL, 9 },
	{ "pex11", NULL, 10 },
	{ "pex12", NULL, 11 },
	{ "pex13", NULL, 12 },
	{ "bp", NULL, 13 },
	{ "sata0lnk", NULL, 14 },
	{ "sata0", "sata0lnk", 15 },
	{ "lcd", NULL, 16 },
	{ "sdio", NULL, 17 },
	{ "usb0", NULL, 18 },
	{ "usb1", NULL, 19 },
	{ "usb2", NULL, 20 },
	{ "xor0", NULL, 22 },
	{ "crypto", NULL, 23 },
	{ "tdm", NULL, 25 },
	{ "pex20", NULL, 26 },
	{ "pex30", NULL, 27 },
	{ "xor1", NULL, 28 },
	{ "sata1lnk", NULL, 29 },
	{ "sata1", "sata1lnk", 30 },
	{ }
};

