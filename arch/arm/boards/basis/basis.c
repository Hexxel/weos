/*
 * \\/ Westermo Basis Platform
 *
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 * 		  Joacim Zetterling <joacim.zetterling@westermo.se>	
 *
 * Copyright (C) 2014 Westermo Teleindustri AB
 *
 * Modified pcm038.c, original copyright follows:
 *
 * Copyright (C) 2007 Sascha Hauer, Pengutronix 
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
 *
 */
#define pr_fmt(fmt) "basis: " fmt

#include <asm/armlinux.h>
#include <bootsource.h>
#include <bootm.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <notifier.h>
#include <linux/sizes.h>
#include <envfs.h>
#include <mach/devices-imx27.h>
#include <mach/weim.h>
#include <mach/iim.h>
#include <mach/imx-pll.h>
#include <mach/imx27-regs.h>
#include <mach/imxfb.h>
#include <mach/iomux-mx27.h>
#include <generated/mach-types.h>
#include <mfd/mc13xxx.h>
#include <mach/bbu.h>
#include <fcntl.h>
#include <fs.h>
#include <globalvar.h>
#include <libwestermo.h>
#include <squashfs_fs.h>

#include "pll.h"

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
	.phy_addr = SC0_SMI_ADDR,
};

static struct i2c_board_info i2c0_bus[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	},
};


/**
 * The spctl0 register is a beast: Seems you can read it
 * only one times without writing it again.
 */
static inline uint32_t get_pll_spctl0(void)
{
	uint32_t reg;

	reg = readl(MX27_CCM_BASE_ADDR + MX27_SPCTL0);
	writel(reg, MX27_CCM_BASE_ADDR + MX27_SPCTL0);

	return reg;
}

/**
 * If the PLL settings are in place switch the CPU core frequency to the max. value
 */
static int basis_pll_init(void)
{
	uint32_t spctl0 = get_pll_spctl0();

	/* PLL registers already set to their final values? */
	if (spctl0 == SPCTL0_VAL &&
	    readl(MX27_CCM_BASE_ADDR + MX27_MPCTL0) == MPCTL0_VAL) {
		console_flush();

		writel(CSCR_VAL,   MX27_CCM_BASE_ADDR + MX27_CSCR);
		writel(0x00008040, MX27_CCM_BASE_ADDR + MX27_MPCTL1);
		writel(0x130410c3, MX27_CCM_BASE_ADDR + MX27_PCDR0);
		writel(0x03030307, MX27_CCM_BASE_ADDR + MX27_PCDR1);

		/* enable: fec, gpio, gpt1, i2c1, imm */
		writel(0x07050000, MX27_CCM_BASE_ADDR + MX27_PCCR0);

		/* enable: uart1, wdt, brom, emi, fec, perclk1 */
		writel(0x814a0400, MX27_CCM_BASE_ADDR + MX27_PCCR1);

		/* Clocks have changed. Notify clients */
		clock_notifier_call_chain();
	}

	/* clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR);

	return 0;
}
// core_initcall(basis_pll_init);


static const struct devfs_partition nor_map[] = {
	{
		.name   = "self0",
		.offset = 0,
		.size   = SZ_512K,
		.flags  = DEVFS_PARTITION_FIXED,
	},
	{
		.name   = "etc",
		.offset =  SZ_32M - SZ_256K,
		.size   =  SZ_256K,
		.flags  = DEVFS_PARTITION_FIXED,
	},

	{ .name = NULL }
};

static struct mv88e6xxx_pdata backplane[] = {
	[0] = {
		.smi_addr = SC0_SMI_ADDR,

		.cpu_speed = 100,

		.cpu_port     = 10,
		.cascade_ports[0] = 9,
		.cascade_ports[1] = -1
	},
	[1] = {
		.smi_addr = SC1_SMI_ADDR,

		.cpu_speed = 1000,

		.cpu_port     = 9,
		.cascade_ports[0] = 8,
		.cascade_ports[1] = -1
	},

	[2] = { .cpu_speed = 0 }
};


static int do_bootm_squashfs(struct image_data *data)
{
	struct squashfs_super_block *img;
	unsigned long imgsz;
	int fd;

	if (!data->initrd_file)
		goto bootz;

	fd = open(data->initrd_file, O_RDONLY);
	if (fd < 0)
		return fd;

	img = memmap(fd, PROT_READ);
	close(fd);
	if (!img)
		return -EIO;

	if (img != (void *)data->initrd_address)
		return -EINVAL;

	imgsz = le64_to_cpu(img->bytes_used);
	data->initrd_res = request_sdram_region("initrd",
						data->initrd_address,
						imgsz);
	if (!data->initrd_res)
		return -ENOMEM;

	data->os_address = PAGE_ALIGN(data->initrd_res->end);

	globalvar_add_simple("linux.bootargs.rdsize",
			     basprintf("ramdisk_size=%lu",
				       (imgsz + SZ_1K - 1) / SZ_1K));
bootz:
	bootm_add_mtdparts();

	return do_bootz_linux(data);
}

static struct image_handler squashfs_handler = {
	.name = "Westermo SquashFS Image",
	.bootm = do_bootm_squashfs,
	.filetype = filetype_arm_zimage,
};

static int basis_devices_init(void)
{
	int i;
	u64 uid = 0;

	unsigned int mode[] = {
		/* FEC */
		PD0_AIN_FEC_TXD0,
		PD1_AIN_FEC_TXD1,
		PD2_AIN_FEC_TXD2,
		PD3_AIN_FEC_TXD3,
		PD4_AOUT_FEC_RX_ER,
		PD5_AOUT_FEC_RXD1,
		PD6_AOUT_FEC_RXD2,
		PD7_AOUT_FEC_RXD3,
		PD8_AF_FEC_MDIO,
		PD9_AIN_FEC_MDC | GPIO_PUEN,
		PD10_AOUT_FEC_CRS,
		PD11_AOUT_FEC_TX_CLK,
		PD12_AOUT_FEC_RXD0,
		PD13_AOUT_FEC_RX_DV,
		PD14_AOUT_FEC_RX_CLK,
		PD15_AOUT_FEC_COL,
		PD16_AIN_FEC_TX_ER,
		PF23_AIN_FEC_TX_EN,
		/* UART1 */
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
		/* I2C1 */
		PD17_PF_I2C_DATA | GPIO_PUEN,
		PD18_PF_I2C_CLK,
		/* I2C2 */
		PC5_PF_I2C2_SDA,
		PC6_PF_I2C2_SCL,
	};

	/* configure 16 bit nor flash on cs0. 0x10*1/133MHz=120ns. */
	imx27_setup_weimcs(0, 0x00001004, 0xa0000d41, 0x001150c0);

	/* xDSL chip on cs1 */
	imx27_setup_weimcs(1, 0x00801d18, 0xa0003d31, 0x00000002);

	/* SHDSL chip on cs3 */
	imx27_setup_weimcs(3, 0x00401d18, 0xa5503b31, 0x00550002);

	/* Clear bit 1 in FMCR to use CS3 instead of secondary SDRAM */
	writel(0xffffffc9, MX27_SYSCTRL_BASE_ADDR + MX27_FMCR);

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	basis_pll_init();

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC0000000, SZ_32M, 0);
	devfs_create_partitions("nor0", nor_map);

	i2c_register_board_info(0, i2c0_bus, ARRAY_SIZE(i2c0_bus));

	imx27_add_i2c0(NULL);
	imx27_add_i2c1(NULL);

	if (imx_iim_read(1, 0, &uid, 6) == 6)
		armlinux_set_serial(uid);

	/* yes, lynx. this was the original name */
	armlinux_set_architecture(MACH_TYPE_LYNX);

	set_product_id_from_idmem();

	/* Register the fec device after the PLL re-initialisation
	 * as the fec depends on the (now higher) ipg clock
	 */
	imx27_add_fec(&fec_info);
	wmo_backplane_setup(backplane);

        register_image_handler(&squashfs_handler);
	return 0;
}

device_initcall(basis_devices_init);

static int basis_console_init(void)
{
	barebox_set_model("Westermo Basis");
	barebox_set_hostname("basis");

	imx27_add_uart0();
	return 0;
}

console_initcall(basis_console_init);

