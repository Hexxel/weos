/*
 * Copyright
 * (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#ifndef __MACH_MVEBU_MSYS_REGS_H
#define __MACH_MVEBU_MSYS_REGS_H

#include <mach/common.h>

#define MSYS_INT_REGS_BASE			IOMEM(MVEBU_REMAP_INT_REG_BASE)
#define MSYS_UART_BASE				(MSYS_INT_REGS_BASE + 0x12000)
#define MSYS_UARTn_BASE(n)			(MSYS_UART_BASE + ((n) * 0x100))

#define MSYS_SYSCTL_BASE			(MSYS_INT_REGS_BASE + 0x18200)
#define MSYS_SOC_CTRL				(MSYS_SYSCTL_BASE + 0x004)
#define  PCIE1_QUADX1_EN			BIT(8)
#define  PCIE0_QUADX1_EN			BIT(7)
#define  PCIE3_EN						BIT(3)
#define  PCIE2_EN						BIT(2)
#define  PCIE1_EN						BIT(1)
#define  PCIE0_EN						BIT(0)
#define MSYS_SAR_BASE				(MSYS_INT_REGS_BASE + 0x18230)
#define  SAR_STAT_0					(MSYS_SAR_BASE + 0x00)
#define  SAR_TCLK_FREQ				BIT(21)
#define  SAR_CPU_FREQ				BIT(18)
#define  SAR_STAT_1					(MSYS_SAR_BASE + 0x04)
#define MSYS_CPU_SOC_ID				(MSYS_SYSCTL_BASE + 0x03c)
#define  CPU_SOC_ID_DEVICE_MASK	0xffff

#define MSYS_PUP_ENABLE				(MSYS_SYSCTL_BASE + 0x44c)
//#define MSYS_PUP_ENABLE_BASE		(MSYS_INT_REGS_BASE + 0x1864c)
#define  GE0_PUP_EN					BIT(0)
#define  GE1_PUP_EN					BIT(1)
#define  LCD_PUP_EN					BIT(2)
#define  NAND_PUP_EN					BIT(4)
#define  SPI_PUP_EN					BIT(5)

#define MSYS_SDRAM_BASE				(MSYS_INT_REGS_BASE + 0x20000)
#define  DDR_BASE_CS					0x180
#define  DDR_BASE_CSn(n)			(DDR_BASE_CS + ((n) * 0x8))
#define  DDR_BASE_CS_HIGH_MASK	0x0000000f
#define  DDR_BASE_CS_LOW_MASK		0xff000000
#define  DDR_SIZE_CS					0x184
#define  DDR_SIZE_CSn(n)			(DDR_SIZE_CS + ((n) * 0x8))
#define  DDR_SIZE_ENABLED			BIT(0)
#define  DDR_SIZE_CS_MASK			0x0000001c
#define  DDR_SIZE_CS_SHIFT			2
#define  DDR_SIZE_MASK				0xff000000

#define MSYS_FABRIC_BASE			(MSYS_INT_REGS_BASE + 0x20200)
#define MSYS_FABRIC_CTRL			(MSYS_FABRIC_BASE + 0x000)
#define  MBUS_ERR_PROP_EN			BIT(8)
#define MSYS_FABRIC_CONF			(MSYS_FABRIC_BASE + 0x004)
#define  FABRIC_NUM_CPUS_MASK		0x3

#define MSYS_TIMER_BASE				(MSYS_INT_REGS_BASE + 0x20300)

#define MSYS_PCIE_UNIT_OFFSET		0x40000
#define MSYS_PCIE_PORT_OFFSET		0x04000
#define MSYS_PCIE_BASE(port)		\
	(MSYS_INT_REGS_BASE + 0x40000 +	\
	(((port) / 4) * MSYS_PCIE_UNIT_OFFSET) +	\
	(((port) % 4) * MSYS_PCIE_PORT_OFFSET))

#define PCIE_DEVICE_VENDOR_ID		0x000

#endif /* __MACH_MVEBU_MSYS_REGS_H */
