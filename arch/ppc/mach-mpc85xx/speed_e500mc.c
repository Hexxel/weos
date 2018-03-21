/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright 2004, 2007-2011 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2003 Motorola Inc.
 * Xianghua Xiao, (X.Xiao@motorola.com)
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <asm/processor.h>
#include <mach/clock.h>
#include <mach/immap_85xx.h>
#include <mach/mpc85xx.h>

void fsl_get_sys_info(struct sys_info *sysInfo)
{
	void __iomem *clk = (void __iomem *)(MPC85xx_CLK_ADDR);
	uint plat_ratio, e500mc_div, pll1_ratio, ddr_ratio;
	int i;

	plat_ratio = in_be32(clk + MPC85xx_CLK_PLLPGSR_OFFSET) & 0x0000007e;
	plat_ratio >>= 1;
	sysInfo->freqSystemBus = plat_ratio * CFG_SYS_CLK_FREQ;

	pll1_ratio = in_be32(clk + MPC85xx_CLK_PLLC1GSR_OFFSET) & 0x000001fe;
	pll1_ratio >>= 1;
	sysInfo->freqPLL1 = pll1_ratio * CFG_SYS_CLK_FREQ;

	for (i = 0; i < fsl_cpu_numcores(); i++) {
		e500mc_div = in_be32(clk + MPC85xx_CLK_CLKC0CSR_OFFSET + i * 0x20);
		e500mc_div = (e500mc_div >> 27) & 0xf;

		sysInfo->freqProcessor[i] = (pll1_ratio * CFG_SYS_CLK_FREQ) / (e500mc_div + 1);
	}

	ddr_ratio = in_be32(clk + MPC85xx_CLK_PLLDGSR_OFFSET) & 0x000001fe;
	ddr_ratio >>= 1;
	sysInfo->freqDDRBus = ddr_ratio * CFG_SYS_CLK_FREQ;
}

unsigned long fsl_get_bus_freq(ulong dummy)
{
	struct sys_info sys_info;

	fsl_get_sys_info(&sys_info);

	return sys_info.freqSystemBus;
}

unsigned long fsl_get_ddr_freq(ulong dummy)
{
	struct sys_info sys_info;

	fsl_get_sys_info(&sys_info);

	return sys_info.freqDDRBus;
}

unsigned long fsl_get_eth_freq(void)
{
	struct sys_info sys_info;

	fsl_get_sys_info(&sys_info);

	return sys_info.freqPLL1 / 2;
}

unsigned long fsl_get_timebase_clock(void)
{
	void __iomem *rcpm = (void __iomem *)(MPC85xx_RCPM_ADDR);
	uint tbclk_div, plat_freq = fsl_get_bus_freq(0);

	tbclk_div = in_be32(rcpm + MPC85xx_RCPM_TBCLKDIV_OFFSET) >> 28;

	switch (tbclk_div) {
	case 0:
		return plat_freq / 16;
	case 2:
		return plat_freq / 8;
	case 6:
		return plat_freq / 24;
	case 8:
		return plat_freq / 32;
	}

	pr_warn("warning: unable to determine timebase\n");
	return plat_freq;
}

unsigned long fsl_get_uart_freq(void)
{
	return fsl_get_bus_freq(0) / 2;
}

unsigned long fsl_get_i2c_freq(void)
{
	return fsl_get_bus_freq(0) / 2;
}
