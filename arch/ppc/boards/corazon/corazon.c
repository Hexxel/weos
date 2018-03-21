/*
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

/* Corazon :
 * The clocks on the Corazon hardware is based on the SYSCLK, 66.67MHz.
 * The SYSCLK produces a 400MHz CCB clock. The local bus clock uses the
 * CCB/16 to produce the LCK. The LCK is 25MHz. The flash read access
 * uses three wait states, 120 ns.
*/

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <globalvar.h>
#include <init.h>
#include <libwestermo.h>
#include <memory.h>
#include <mv88e6xxx.h>
#include <platform_data/serial-ns16550.h>
#include <partition.h>
#include <poller.h>
#include <linux/sizes.h>
#include <types.h>
#include <watchdog.h>

#include <asm/cache.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_law.h>

#include <generated/utsrelease.h>

#include <i2c/i2c.h>

#include <linux/phy.h>

#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>
#include <mach/clock.h>
#include <mach/early_udelay.h>

#define GPIO_WATCHDOG  (1 << (31 - 6))
#define GPIO_SYS_RESET (1 << (31 - 7))

#define WATCHDOG_INTERVAL (500 * MSECOND)

struct wd_gpio {
	struct watchdog wd;

	struct poller_struct poll;
	u64 timeout;

	void __iomem *gpdat;
	int next;
};

static void wd_gpio_kick(struct poller_struct *poll)
{
	struct wd_gpio *priv = container_of(poll, struct wd_gpio, poll);
	u64 now = get_time_ns();

	if (now < priv->timeout)
		return;

	if (priv->next)
		setbits_be32(priv->gpdat, GPIO_WATCHDOG);
	else
		clrbits_be32(priv->gpdat, GPIO_WATCHDOG);

	priv->next = !priv->next;
	priv->timeout = now + WATCHDOG_INTERVAL;
}

static int wd_gpio_init(struct watchdog *wd, unsigned timeout)
{
	struct wd_gpio *priv = container_of(wd, struct wd_gpio, wd);
	void __iomem *gpio_base  = (void __iomem *)MPC85xx_GPIO_ADDR;
	void __iomem *gpdir = gpio_base + MPC85xx_GPIO_GPDIR;

	priv->gpdat = gpio_base +  MPC85xx_GPIO_GPDAT;

	setbits_be32(gpdir,       GPIO_WATCHDOG);
	clrbits_be32(priv->gpdat, GPIO_WATCHDOG);
	priv->next = 1;

	priv->timeout = get_time_ns() + WATCHDOG_INTERVAL;
	return poller_register(&priv->poll);
}

static struct wd_gpio wd_gpio = {
	.wd = {
		.set_timeout = wd_gpio_init,
	},
	.poll = {
		.func = wd_gpio_kick,
	},
};

const struct devfs_partition nor_map[] = {
	{
		.name   = "etc",
		.offset = -SZ_1M,
		.size   =  SZ_512K,
		.flags  = DEVFS_PARTITION_FIXED,
	},
	{
		.name   = "self0",
		.offset = -SZ_512K,
		.size   =  SZ_512K,
		.flags  = DEVFS_PARTITION_FIXED,
	},

	{ .name = NULL }
};

static struct gfar_info_struct gfar_info = {
	.phyaddr = SC0_SMI_ADDR,
	.tbiana = 0,
	.tbicr = 0,
	.mdiobus_tbi = 0,
};

static struct i2c_board_info i2c0_bus[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	},
};

struct i2c_platform_data i2cplat = {
	.bitrate = 400000,
};

struct part_fixup_map {
	char *part;
	char *from;
	char *to;
};

static struct part_fixup_map part_fixup_map[] = {
	{
		.part = "partition@7F00000",
		.from = "U-Boot Config",
		.to   = "BareboxEnv",
	},
	{
		.part = "partition@7F80000",
		.from = "U-Boot",
		.to   = "Barebox",
	},

	{ NULL }
};

static struct mv88e6xxx_pdata backplane[] = {
	[0] = {
		.smi_addr = SC0_SMI_ADDR,

		.cpu_speed = 1000,

		.cpu_port     = 5,
		.cascade_ports[0] = 4,
		.cascade_ports[1] = -1
	},
	[1] = {
		.smi_addr = SC1_SMI_ADDR,

		.cpu_speed = 1000,

		.cpu_port     = 8,
		.cascade_ports[0] = 10,
		.cascade_ports[1] = -1
	},
	[2] = {
		.smi_addr = SC2_SMI_ADDR,

		.cpu_speed = 1000,

		.cpu_port     = 8,
		.cascade_ports[0] = 10,
		.cascade_ports[1] = -1
	},
	[3] = {
		.smi_addr = SC3_SMI_ADDR,

		.cpu_speed = 1000,

		.cpu_port     = 8,
		.cascade_ports[0] = 10,
		.cascade_ports[1] = -1
	},

	[4] = { .cpu_speed = 0 }
};

static int corazon_of_fixup(struct device_node *root, void *arg)
{
	struct part_fixup_map *fixup;
	struct device_node *np;
	const char *label;

	/* Convert 4.x images partition map to Barebox since that is
	 * obviously what we are running */
	for (fixup = part_fixup_map; fixup->part; fixup++) {
		np = of_find_node_by_name(root, fixup->part);
		if (!np)
			continue;

		if (of_property_read_string(np, "label", &label))
			continue;

		if (strcmp(label, fixup->from))
			continue;

		of_property_write_u8_array(np, "label", fixup->to,
					   strlen(fixup->to) + 1);
	}

	return 0;
}

static int devices_init(void)
{
	watchdog_register(&wd_gpio.wd);

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, CFG_FLASH_BASE, SZ_128M, 0);
	devfs_create_partitions("nor0", nor_map);

	i2c_register_board_info(0, i2c0_bus, ARRAY_SIZE(i2c0_bus));

	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR,
			0x100, IORESOURCE_MEM, &i2cplat);
	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR,
			0x100, IORESOURCE_MEM, &i2cplat);

	set_product_id_from_idmem();
	wmo_backplane_setup(backplane);
	fsl_eth_init(1, &gfar_info);

	of_register_fixup(corazon_of_fixup, NULL);
	return 0;
}

device_initcall(devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int corazon_console_init(void)
{
	barebox_set_model("Westermo Corazon");
	barebox_set_hostname("corazon");

	serial_plat.clock = fsl_get_bus_freq(0);

	add_ns16550_device(DEVICE_ID_DYNAMIC, 0xffe04500, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(corazon_console_init);

static int mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, 512 << 20);

	return 0;
}
mem_initcall(mem_init);

void deassert_sys_reset(void)
{
	void __iomem *gpio_base  = (void __iomem *)MPC85xx_GPIO_ADDR;
	void __iomem *gpio_gpdat = gpio_base +  MPC85xx_GPIO_GPDAT;
	void __iomem *gpio_gpdir = gpio_base +  MPC85xx_GPIO_GPDIR;
	void __iomem *gpio_gpodr = gpio_base +  MPC85xx_GPIO_GPODR;

	/* output/open-drain/drive low */
	setbits_be32(gpio_gpdat, GPIO_SYS_RESET);
	setbits_be32(gpio_gpdir, GPIO_SYS_RESET);
	setbits_be32(gpio_gpodr, GPIO_SYS_RESET);
}

phys_size_t fixed_sdram(void)
{
	void __iomem *regs = (void __iomem *)(MPC85xx_DDR_ADDR);
	int sdram_cfg = (SDRAM_CFG_MEM_EN | SDRAM_CFG_SREN |
			 SDRAM_CFG_SDRAM_TYPE_DDR3 | SDRAM_CFG_32_BE |
			 SDRAM_CFG_8_BE | SDRAM_CFG_HSE);
	phys_size_t dram_size;

	/* If already enabled (running from RAM), get out */
	if (in_be32(regs + DDR_OFF(SDRAM_CFG)) & SDRAM_CFG_MEM_EN)
		return fsl_get_effective_memsize();

	out_be32(regs + DDR_OFF(CS0_BNDS), CFG_SYS_DDR_CS0_BNDS);
	out_be32(regs + DDR_OFF(CS0_CONFIG), CFG_SYS_DDR_CS0_CONFIG);
	out_be32(regs + DDR_OFF(TIMING_CFG_3), CFG_SYS_DDR_TIMING_3);
	out_be32(regs + DDR_OFF(TIMING_CFG_0), CFG_SYS_DDR_TIMING_0);
	out_be32(regs + DDR_OFF(TIMING_CFG_1), CFG_SYS_DDR_TIMING_1);
	out_be32(regs + DDR_OFF(TIMING_CFG_2), CFG_SYS_DDR_TIMING_2);
	out_be32(regs + DDR_OFF(SDRAM_CFG_2), CFG_SYS_DDR_CONTROL2);
	out_be32(regs + DDR_OFF(SDRAM_MODE), CFG_SYS_DDR_MODE_1);
	out_be32(regs + DDR_OFF(SDRAM_MODE_2), CFG_SYS_DDR_MODE_2);
	out_be32(regs + DDR_OFF(SDRAM_MD_CNTL), CFG_SYS_MD_CNTL);
	/* Basic refresh rate (7.8us),high temp is 3.9us  */
	out_be32(regs + DDR_OFF(SDRAM_INTERVAL),
			CFG_SYS_DDR_INTERVAL);
	out_be32(regs + DDR_OFF(SDRAM_DATA_INIT),
			CFG_SYS_DDR_DATA_INIT);
	out_be32(regs + DDR_OFF(SDRAM_CLK_CNTL),
			CFG_SYS_DDR_CLK_CTRL);

	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR), 0);
	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR_EXT), 0);
	/*
	 * Wait 200us for the DDR clock to stabilize.
	 */
	early_udelay(200);
	asm volatile ("sync;isync");

	out_be32(regs + DDR_OFF(SDRAM_CFG), sdram_cfg);

	dram_size = fsl_get_effective_memsize();
	if (fsl_set_ddr_laws(0, dram_size, LAW_TRGT_IF_DDR) < 0)
		return 0;

	return dram_size;
}

static int board_init_r(void)
{
	const u8 flash_low  = e500_find_tlb_idx((void *)CFG_FLASH_LOW,  1);
	const u8 flash_high = e500_find_tlb_idx((void *)CFG_FLASH_HIGH, 1);

	deassert_sys_reset();

	/* Map FLASH */
	fsl_set_lbc_br(0, BR_PHYS_ADDR(CFG_FLASH_BASE_PHYS) | BR_PS_16 | BR_V);
	fsl_set_lbc_or(0, 0xf8000d30); /* use 3 ws to handle the 120 ns flashes. */

	/* Map FPGA */
	fsl_set_lbc_br(1, BR_PHYS_ADDR(CFG_FPGA_BASE_PHYS) | BR_PS_16 | BR_V);
	fsl_set_lbc_or(1, 0xffff0ff7);

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash */
	e500_disable_tlb(flash_low);
	e500_disable_tlb(flash_high);

	/*
	 * Remap Boot flash region to caching-inhibited
	 * so that flash can be erased properly.
	 */
	e500_set_tlb(1, CFG_FLASH_LOW, CFG_FLASH_LOW,
		MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		0, flash_low, BOOKE_PAGESZ_64M, 1);

	e500_set_tlb(1, CFG_FLASH_HIGH, CFG_FLASH_HIGH,
		MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		0, flash_high, BOOKE_PAGESZ_64M, 1);

	fsl_l2_cache_init();

	return 0;
}
core_initcall(board_init_r);

void __noreturn reset_cpu(unsigned long addr)
{
	void __iomem *gpio_base  = (void __iomem *)MPC85xx_GPIO_ADDR;
	void __iomem *gpio_gpdat = gpio_base +  MPC85xx_GPIO_GPDAT;

	clrbits_be32(gpio_gpdat, GPIO_SYS_RESET);
	while (1);
}
