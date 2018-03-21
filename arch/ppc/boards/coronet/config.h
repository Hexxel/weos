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
 * Coronet board configuration file
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_SYS_CLK_FREQ   100000000
/*
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
 */
#define CFG_CCSRBAR_DEFAULT     0xfe000000
#define CFG_CCSRBAR             CFG_CCSRBAR_DEFAULT
#define CFG_IMMR                CFG_CCSRBAR

#ifdef CONFIG_ENABLE_36BIT_PHYS
#define CFG_CCSRBAR_PHYS_HIGH   0xf
#define CFG_CCSRBAR_PHYS_LOW    CFG_CCSRBAR_DEFAULT

#define CFG_CCSRBAR_PHYS ((CFG_CCSRBAR_PHYS_HIGH * 1ull) << 32 | \
             CFG_CCSRBAR_PHYS_LOW)
#else
#define CFG_CCSRBAR_PHYS	CFG_CCSRBAR_DEFAULT
#endif



/* DDR Setup */

#define CFG_CHIP_SELECTS_PER_CTRL   1

#define CFG_SDRAM_BASE			0x00000000

#define CFG_SYS_DDR_CS0_BNDS		0x0000001f	/* 512M */
#define CFG_SYS_DDR_CS0_CONFIG		0x80014202
#define CFG_SYS_DDR_CS1_BNDS		0x00000000
#define CFG_SYS_DDR_CS1_CONFIG		0x00000000
#define CFG_SYS_DDR_TIMING_3		0x01061000
#define CFG_SYS_DDR_TIMING_0		0x4044000C
#define CFG_SYS_DDR_TIMING_1		0xa8a03a45
#define CFG_SYS_DDR_TIMING_2		0x0038b11b
#define CFG_SYS_DDR_TIMING_3		0x01061000
#define CFG_SYS_DDR_TIMING_4		0x00000001
#define CFG_SYS_DDR_TIMING_5		0x03401400
#define CFG_SYS_DDR_TIMING_6		0x00000000
#define CFG_SYS_DDR_TIMING_7		0x00000000
#define CFG_SYS_DDR_TIMING_8		0x00000000

#define CFG_SYS_DDR_ZQ_CONTROL		0x89080600
#define CFG_SYS_DDR_WRLVL_CONTROL	0x8655F604
#define CFG_SYS_DDR_WRLVL_CONTROL_2	0x04040400
#define CFG_SYS_DDR_WRLVL_CONTROL_3	0x00000000
#define CFG_SYS_DDR_CONTROL		0x672C000C
#define CFG_SYS_DDR_CONTROL2		0x00401010
#define CFG_SYS_DDR_MODE_1		0x00441A50
#define CFG_SYS_DDR_MODE_2		0x00100000
#define CFG_SYS_MD_CNTL			0x00000000
#define CFG_SYS_DDR_INTERVAL		0x0A270289
#define CFG_SYS_SR_CNTL			0x00000000

#define CFG_SYS_DDR_SDRAM_MODE_3	0x0
#define CFG_SYS_DDR_SDRAM_MODE_4	0x0
#define CFG_SYS_DDR_SDRAM_MODE_5	0x0
#define CFG_SYS_DDR_SDRAM_MODE_6	0x0
#define CFG_SYS_DDR_SDRAM_MODE_7	0x0
#define CFG_SYS_DDR_SDRAM_MODE_8	0x0
#define CFG_SYS_DDR_SDRAM_MODE_9	0x0
#define CFG_SYS_DDR_SDRAM_MODE_10	0x0

#define CFG_SYS_DDR_DDRCDR1		0x80040000
#define CFG_SYS_DDR_DDRCDR2		0x00000001

#define CFG_SYS_DDR_DATA_INIT		0xdeadbeef
#define CFG_SYS_DDR_CLK_CTRL		0x01c00000

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
#define CFG_SIZE_64MB               (64 << 20)
#define CFG_FLASH_BASE              0xe8000000
#define CFG_FLASH_LOW               CFG_FLASH_BASE
#define CFG_FLASH_HIGH              (CFG_FLASH_LOW + CFG_SIZE_64MB)
#ifdef CONFIG_ENABLE_36BIT_PHYS
#define CFG_FLASH_PHYS_LOW          (0xf00000000ull | CFG_FLASH_LOW)
#define CFG_FLASH_PHYS_HIGH         (0xf00000000ull | CFG_FLASH_HIGH)
#else
#define CFG_FLASH_PHYS_LOW          CFG_FLASH_LOW
#define CFG_FLASH_PHYS_HIGH         CFG_FLASH_HIGH
#endif
#define CFG_FLASH_BASE_PHYS         CFG_FLASH_PHYS_LOW

#define CFG_FPGA_BASE	      	0xffa00000
#define CFG_FPGA_BASE_PHYS	CFG_FPGA_BASE

#define CFG_INIT_RAM_ADDR	0xffd00000	/* stack in RAM */
/* Leave 256 bytes for global data */
#define CFG_INIT_SP_OFFSET	(0x00004000 - 256)
#define CFG_BMAN_NUM_PORTALS	10
#ifdef CONFIG_ENABLE_36BIT_PHYS
#define CFG_BMAN_BASE_PHYS          0xff4000000ull
#define CFG_BMAN_BASE_PHYS_HIGH     0xff5000000ull
#else
#define CFG_BMAN_BASE_PHYS          0xf4000000
#define CFG_BMAN_BASE_PHYS_HIGH     0xf5000000
#endif
#define CFG_BMAN_MEM_SIZE       0x02000000
#define CFG_BMAN_SP_CENA_SIZE   0x4000
#define CFG_BMAN_SP_CINH_SIZE   0x1000
#define CFG_BMAN_MEM_BASE      	CFG_BMAN_BASE_PHYS
#define CFG_BMAN_CENA_BASE      CFG_BMAN_MEM_BASE
#define CFG_BMAN_CENA_SIZE      (CFG_BMAN_MEM_SIZE >> 1)
#define CFG_BMAN_CINH_BASE      (CFG_BMAN_MEM_BASE +    \
                                 CFG_BMAN_CENA_SIZE)
#define CFG_BMAN_CINH_SIZE      (CFG_BMAN_MEM_SIZE >> 1)
#define CFG_BMAN_SWP_ISDR_REG	0xE08

#ifdef CONFIG_ENABLE_36BIT_PHYS
#define CFG_QMAN_BASE_PHYS          0xff6000000ull
#define CFG_QMAN_BASE_PHYS_HIGH     0xff7000000ull
#else
#define CFG_QMAN_BASE_PHYS          0xf6000000
#define CFG_QMAN_BASE_PHYS_HIGH     0xf7000000
#endif
#define CFG_QMAN_NUM_PORTALS	10
#define CFG_QMAN_MEM_SIZE	0x02000000
#define CFG_QMAN_SP_CENA_SIZE   0x4000
#define CFG_QMAN_SP_CINH_SIZE   0x1000
#define CFG_QMAN_MEM_BASE       CFG_QMAN_BASE_PHYS
#define CFG_QMAN_CENA_BASE      CFG_QMAN_MEM_BASE
#define CFG_QMAN_CENA_SIZE      (CFG_QMAN_MEM_SIZE >> 1)
#define CFG_QMAN_CINH_BASE      (CFG_QMAN_MEM_BASE +    \
                                 CFG_QMAN_CENA_SIZE)
#define CFG_QMAN_CINH_SIZE      (CFG_QMAN_MEM_SIZE >> 1)
#define CFG_QMAN_SWP_ISDR_REG	0xE08

#endif	/* __CONFIG_H */
