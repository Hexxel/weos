/*
 * Copyright 2014 Westermo Teleindustri AB
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 */

/*
 * Corazon board configuration file
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_SYS_CLK_FREQ	66666666
#define CFG_DDR_CLK_FREQ	66666666

/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT	0xff700000

#define CFG_CCSRBAR		0xffe00000	/* relocated CCSRBAR */
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR

#define CFG_IMMR		CFG_CCSRBAR

/* DDR Setup */

#define CFG_CHIP_SELECTS_PER_CTRL   1

#define CFG_SDRAM_BASE		0x00000000

/* These timings are adjusted for a 667Mhz clock. */
#define CFG_SYS_DDR_CS0_BNDS		0x0000001f	/* 512M */
#define CFG_SYS_DDR_CS0_CONFIG	        0x80014302
#define CFG_SYS_DDR_TIMING_3		0x00020000
#define CFG_SYS_DDR_TIMING_0		0x00110104
#define CFG_SYS_DDR_TIMING_1		0x5d59e544
#define CFG_SYS_DDR_TIMING_2		0x0fa888cd

#define CFG_SYS_DDR_CONTROL		0x470c0008
#define CFG_SYS_DDR_CONTROL2		0x24401000
#define CFG_SYS_DDR_MODE_1		0x00441210
#define CFG_SYS_DDR_MODE_2		0x00000000
#define CFG_SYS_MD_CNTL			0x00000000
#define CFG_SYS_DDR_INTERVAL		0x0a280100

#define CFG_SYS_DDR_DATA_INIT	        0xdeadbeef
#define CFG_SYS_DDR_CLK_CTRL		0x03000000

/*
 * Memory map
 *
 * 0x0000_0000	0x3fff_ffff	DDR			1G cacheablen
 *
 * Localbus non-cacheable
 * 0xef00_0000	0xefff_ffff	FLASH			16M non-cacheable
 * 0xffd0_0000	0xffd0_3fff	L1 for stack		16K Cacheable TLB0
 */

/*
 * Local Bus Definitions
 */
#define CFG_FLASH_BASE		0xe8000000
#define CFG_FLASH_BASE_PHYS	CFG_FLASH_BASE
#define CFG_FLASH_LOW           CFG_FLASH_BASE
#define CFG_FLASH_HIGH          (CFG_FLASH_LOW + (64 << 20))

#define CFG_FPGA_BASE		0xffa00000
#define CFG_FPGA_BASE_PHYS	CFG_FPGA_BASE

#define CFG_INIT_RAM_ADDR	0xffd00000	/* stack in RAM */
/* Leave 256 bytes for global data */
#define CFG_INIT_SP_OFFSET	(0x00004000 - 256)

#endif	/* __CONFIG_H */
