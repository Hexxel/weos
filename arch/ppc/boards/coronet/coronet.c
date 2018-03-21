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

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <globalvar.h>
#include <init.h>
#include <libwestermo.h>
#include <memory.h>
#include <mv88e6xxx.h>
#include <config/driver/serial/ns16550.h>
#include <partition.h>
#include <poller.h>
#include <linux/sizes.h>
#include <types.h>
#include <watchdog.h>
#include <fm_eth.h>
#include <linux/phy.h>
#include <led.h>
#include <platform_data/serial-ns16550.h>
#include "../../../../drivers/net/fm/fm.h"

#include <asm/cache.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ifc.h>
#include <asm/fsl_law.h>

#include <generated/utsrelease.h>

#include <i2c/i2c.h>

#include <linux/phy.h>

#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>
#include <mach/memac.h>
#include <mach/clock.h>
#include <mach/early_udelay.h>
#include <mach/fsl_usb.h>
#include <asm/fsl_portals.h>
#include <asm/fsl_liodn.h>

#define GPIO_WATCHDOG  (1 << (31 - 16))

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

void __noreturn reset_cpu(unsigned long addr)
{
	poller_unregister(&wd_gpio.poll);
	while (1);
}

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

static struct memac_mdio_pdata memac_mdio_pdata = {
};

static struct fsl_usb_phy_pdata usb_phy = {
	.port1 = {
		.enable = 1,
		.pwrflt_act_low = 1,
	}
};

static struct i2c_board_info i2c0_bus[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	},
};

static struct i2c_board_info i2c1_bus[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	},
	{
		I2C_BOARD_INFO("max7313", 0x20)
	},

};

static struct i2c_board_info i2c2_bus[] = {
	{
		I2C_BOARD_INFO("PD69104B1F", 0x20)
	},
	{
		I2C_BOARD_INFO("PD69104B1F", 0x24)
	},

};

struct i2c_platform_data i2cplat = {
	.bitrate = 100000,
};

static struct mv88e6xxx_pdata backplane_cascade[] = {
	[0] = {
		.smi_addr = SC0_SMI_ADDR,

		.cpu_speed        = 1000,

		.cpu_port         = 6,
		.cascade_ports[0] = 5,
		.cascade_ports[1] = -1,
		.cascade_rgmii[0] = 1,
		.cascade_rgmii[1] = -1
	},

	[1] = {
		.smi_addr = SC1_SMI_ADDR,

		.cpu_speed        = 1000,

		.cpu_port         = 10,
		.cascade_ports[0] = 9,
		.cascade_ports[1] = -1
	},

	[2] = { .cpu_speed = 0 }
};

static struct mv88e6xxx_pdata backplane_star[] = {
	[0] = {
		.smi_addr = SC0_SMI_ADDR,

		.cpu_speed        = 1000,

		.cpu_port         = 6,
		.cascade_ports[0] = 5,
		.cascade_ports[1] = 4,
		.cascade_rgmii[0] = 1,
		.cascade_rgmii[1] = -1
	},

	[1] = {
		.smi_addr = SC1_SMI_ADDR,

		.cpu_speed        = 1000,

		.cpu_port         = 10,
		.cascade_ports[0] = -1,
		.cascade_ports[1] = -1
	},

	[2] = {
		.smi_addr = SC3_SMI_ADDR,

		.cpu_speed        = 1000,

		.cpu_port         = 10,
		.cascade_ports[0] = -1,
		.cascade_ports[1] = -1
	},

	[3] = { .cpu_speed = 0 }
};

struct fm_eth_info fm_info[] = {
	FM_DTSEC_INFO_INITIALIZER(1, 4),
};

#define ON_LED   0
#define FRNT_LED 2
#define RSTP_LED 4
#define USR1_LED 6
struct gpio_bicolor_led leds[] = {
	{
		.gpio_c0 = ON_LED,
		.gpio_c1 = ON_LED + 1,
		.active_low = true,
		.led.name = "on",
	},
	{},
	{
		.gpio_c0 = FRNT_LED,
		.gpio_c1 = FRNT_LED + 1,
		.active_low = true,
		.led.name = "frnt",
	},
	{},
	{
		.gpio_c0 = RSTP_LED,
		.gpio_c1 = RSTP_LED + 1,
		.active_low = true,
		.led.name = "rstp",
	},
	{},
	{
		.gpio_c0 = USR1_LED,
		.gpio_c1 = USR1_LED + 1,
		.active_low = true,
		.led.name = "usr1",
	},
};

static int devices_init(void)
{
	int i = 0;
	struct led *led;

	watchdog_register(&wd_gpio.wd);

	add_cfi_flash_device(DEVICE_ID_DYNAMIC, CFG_FLASH_BASE, SZ_128M, 0);
	devfs_create_partitions("nor0", nor_map);

	i2c_register_board_info(0, i2c0_bus, ARRAY_SIZE(i2c0_bus));

	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR,
			   0x100, IORESOURCE_MEM, &i2cplat);

	i2c_register_board_info(1, i2c1_bus, ARRAY_SIZE(i2c1_bus));

	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR,
			   0x100, IORESOURCE_MEM, &i2cplat);

	i2c_register_board_info(2, i2c2_bus, ARRAY_SIZE(i2c2_bus));

	add_generic_device("i2c-fsl", 2, NULL, I2C3_BASE_ADDR,
			   0x100, IORESOURCE_MEM, &i2cplat);

	fsl_usb_phy_init(&usb_phy);

	led_gpio_bicolor_register (&leds[ON_LED]);
	led_gpio_bicolor_register (&leds[FRNT_LED]);
	led_gpio_bicolor_register (&leds[RSTP_LED]);
	led_gpio_bicolor_register (&leds[USR1_LED]);

	led = led_by_name_or_number("on");
	if (!led)
		pr_err("ON LED not found\n");
	else
		led_set(led, 2);

	memac_mdio_pdata.enet_freq = fsl_get_eth_freq();
	add_generic_device("memac-mdio", 0, NULL, 0xfe4fc030,
			   0x10, IORESOURCE_MEM, &memac_mdio_pdata);

   set_product_id_from_idmem();

   if (product_is_coronet_star())
	   wmo_backplane_setup(backplane_star);
   else
	   wmo_backplane_setup(backplane_cascade);

	fm_info[0].phy_addr = 4;
	fm_info[0].enet_if = PHY_INTERFACE_MODE_RGMII;
	for (; i < ARRAY_SIZE(fm_info); i++) {
		add_generic_device("fm_eth", 0, NULL, MPC85xx_FMAN_FM1_ADDR,
				   0x10, IORESOURCE_MEM, &fm_info[i]);
	}

	/* of_register_fixup(coronet_of_fixup, NULL); */
	return 0;
}

device_initcall(devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int coronet_console_init(void)
{
	barebox_set_model("Westermo Coronet");
	barebox_set_hostname("coronet");

	serial_plat.clock = fsl_get_uart_freq();

	add_ns16550_device(DEVICE_ID_DYNAMIC, 0xfe11c500, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(coronet_console_init);

static int mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, 512 << 20);

	return 0;
}
mem_initcall(mem_init);

static void errata_A009942(void __iomem *regs)
{
	/*Start Errata A-009942: set DEBUG29 to 0080006ah when DDR operating at 1333*/
	/*T1024, T1014, T1023, and T1013 Chip Errata, Rev. 3, 02/2016*/
	out_be32(regs + DDR_OFF(DEBUG29), 0x0080006A);
}

static void errata_A009988(void __iomem *regs)
{
	u32 dbg21,dbg22,dbg21a,dbg21b,dbg21c,dbg22a,dbg22b;

	/*Start Errata A-009988: */
	/*T1024, T1014, T1023, and T1013 Chip Errata, Rev. 3, 02/2016*/
	/*1. Configure DDR registers, which is done above*/
	/* 2. Write 0 to DDR_SDRAM_CFG_2[D_INIT] */
	out_be32(regs + DDR_OFF(SDRAM_CFG_2),(in_be32(regs + DDR_OFF(SDRAM_CFG_2)) & 0xFFFFFFEF));

	/* 3. Write 0x00000400 to DEBUG3 */
	out_be32(regs + DDR_OFF(DEBUG3),0x00000400);

	/* 4. Write 0xff800000 to DEBUG5 */
	out_be32(regs + DDR_OFF(DEBUG5),0xff800000);

	/* 5. Write 0 to DDRCDR2[24] */
	out_be32(regs + DDR_OFF(DDRCDR2),(in_be32(regs + DDR_OFF(DDRCDR2)) & 0xFFFFFF7F));

	/* 6. Write 0 to DDR_SDRAM_CFG[ECC_EN] */
	out_be32(regs + DDR_OFF(SDRAM_CFG),(in_be32(regs + DDR_OFF(SDRAM_CFG)) & 0xDFFFFFFF));

	/* 7. Write 1 to DDR_SDRAM_CFG[MEM_EN] */
	out_be32(regs + DDR_OFF(SDRAM_CFG),(in_be32(regs + DDR_OFF(SDRAM_CFG)) | 0x80000000));

	/* 8. Poll for DEBUG2[30] to be set */
	while (1)
	{
	   if ((in_be32(regs + DDR_OFF(DEBUG2)) & 0x00000002) > 0)
		   break;
	}

	/* 9. Read DEBUG21 and use the value in later steps */
	dbg21 = in_be32(regs + DDR_OFF(DEBUG21));

	/* 10. Read DEBUG22 and use the value in later steps */
	dbg22 = in_be32(regs + DDR_OFF(DEBUG22));

	/* 11. Write 0 to DDR_SDRAM_CFG[MEM_EN] */
	out_be32(regs + DDR_OFF(SDRAM_CFG),(in_be32(regs + DDR_OFF(SDRAM_CFG)) & 0x7FFFFFFF));

	/* 12. Copy DEBUG21[2:7] to {TIMING_CFG_2[22,19:21] ,DEBUG14[22:23]} */
	dbg21a = ((dbg21 & 0x20000000) >> 20);
	dbg21b = ((dbg21 & 0x1C000000) >> 16);
	dbg21c = ((dbg21 & 0x03000000) >> 16);
	out_be32(regs + DDR_OFF(TIMING_CFG_2),((in_be32(regs + DDR_OFF(TIMING_CFG_2)) & 0xFFFFE1FF) | (dbg21a | dbg21b)));
	out_be32(regs + DDR_OFF(DEBUG14),((in_be32(regs + DDR_OFF(DEBUG14)) & 0xFFFFFCFF) | dbg21c));

	/* 13. Copy DEBUG21[18:23] to DEBUG15[2:7] */
	dbg21a = ((dbg21 & 0x00003F00) << 16);
	out_be32(regs + DDR_OFF(DEBUG15),((in_be32(regs + DDR_OFF(DEBUG15)) & 0xC0FFFFFF) | dbg21a));

	/* 14. Copy DEBUG22[2:7] to DEBUG15[18:23] */
	dbg22a = ((dbg22 & 0x3F000000) >> 16);
	out_be32(regs + DDR_OFF(DEBUG15),((in_be32(regs + DDR_OFF(DEBUG15)) & 0xFFFFC0FF) | dbg22a));

	/* 15. Write 1 to DEBUG15[0] and DEBUG15[16] */
	out_be32(regs + DDR_OFF(DEBUG15),(in_be32(regs + DDR_OFF(DEBUG15)) | 0x80008000));

	/* 16. Copy DEBUG22[18:23] to DEBUG16[2:7] */
	dbg22a = ((dbg22 & 0x00003F00) << 16);
	out_be32(regs + DDR_OFF(DEBUG16),((in_be32(regs + DDR_OFF(DEBUG16)) & 0xC0FFFFFF) | dbg22a));

	/* 17. Write 1 to DEBUG16[0] */
	out_be32(regs + DDR_OFF(DEBUG16),(in_be32(regs + DDR_OFF(DEBUG16)) | 0x80000000));

	/* 18. If ECC byte is closer to byte lane 3,
	add 1 to DEBUG22[18:23] (keep in mind that DEBUG22 values was obtained in step 10) and put the result in DEBUG18[18:23] */
	dbg22a = ((dbg22 & 0x00003F00) >> 8);
	dbg22b = ((dbg22a + 1) << 8);
	out_be32(regs + DDR_OFF(DEBUG18),((in_be32(regs + DDR_OFF(DEBUG18)) & 0xFFFFC0FF) | dbg22b));

	/* 19. Write 1 to DEBUG18[16] */
	out_be32(regs + DDR_OFF(DEBUG18),(in_be32(regs + DDR_OFF(DEBUG18)) | 0x00008000));

	/* 20. Write 1 to DDR_SDRAM_CFG_2[D_INIT] */
	out_be32(regs + DDR_OFF(SDRAM_CFG_2),(in_be32(regs + DDR_OFF(SDRAM_CFG_2)) | 0x00000010));

	/* 21. Write 0 to DEBUG3 */
	out_be32(regs + DDR_OFF(DEBUG3),0);

	/* 22. Write 0 to DEBUG5 */
	out_be32(regs + DDR_OFF(DEBUG5),0);

	/* 23. Since DRAM VRef training is not used write 1 to DDRCDR_2[24] is not done */

	/* 24. Write 0 to DDR_WRLVL_CNTL[WRLVL_EN] */
	out_be32(regs + DDR_OFF(WRLVL_CNTL),(in_be32(regs + DDR_OFF(WRLVL_CNTL)) & 0x7FFFFFFF));

	/* 25. write 1 to DDR_SDRAM_CFG[ECC_EN] */
	out_be32(regs + DDR_OFF(SDRAM_CFG),(in_be32(regs + DDR_OFF(SDRAM_CFG)) | 0x20000000));

	/* 26. write 1 to DDR_SDRAM_CFG[MEM_EN] */
	out_be32(regs + DDR_OFF(SDRAM_CFG),(in_be32(regs + DDR_OFF(SDRAM_CFG)) | 0x80000000));
}

phys_size_t fixed_sdram(void)
{
	phys_size_t dram_size;
	void __iomem *regs = (void __iomem *)(MPC85xx_DDR_ADDR);

	/* If already enabled (running from RAM), get out */
	if (in_be32(regs + DDR_OFF(SDRAM_CFG)) & SDRAM_CFG_MEM_EN)
		return fsl_get_effective_memsize();

	out_be32(regs + DDR_OFF(SDRAM_CFG), CFG_SYS_DDR_CONTROL);
	out_be32(regs + DDR_OFF(CS0_BNDS), CFG_SYS_DDR_CS0_BNDS);
	out_be32(regs + DDR_OFF(CS0_CONFIG), CFG_SYS_DDR_CS0_CONFIG);
	out_be32(regs + DDR_OFF(CS1_BNDS), CFG_SYS_DDR_CS1_BNDS);
	out_be32(regs + DDR_OFF(CS1_CONFIG), CFG_SYS_DDR_CS1_CONFIG);
	out_be32(regs + DDR_OFF(TIMING_CFG_3), CFG_SYS_DDR_TIMING_3);
	out_be32(regs + DDR_OFF(TIMING_CFG_0), CFG_SYS_DDR_TIMING_0);
	out_be32(regs + DDR_OFF(TIMING_CFG_1), CFG_SYS_DDR_TIMING_1);
	out_be32(regs + DDR_OFF(TIMING_CFG_2), CFG_SYS_DDR_TIMING_2);
	out_be32(regs + DDR_OFF(TIMING_CFG_4), CFG_SYS_DDR_TIMING_4);
	out_be32(regs + DDR_OFF(TIMING_CFG_5), CFG_SYS_DDR_TIMING_5);
	out_be32(regs + DDR_OFF(TIMING_CFG_6), CFG_SYS_DDR_TIMING_6);
	out_be32(regs + DDR_OFF(TIMING_CFG_7), CFG_SYS_DDR_TIMING_7);
	out_be32(regs + DDR_OFF(TIMING_CFG_8), CFG_SYS_DDR_TIMING_8);
	out_be32(regs + DDR_OFF(SDRAM_CFG_2), CFG_SYS_DDR_CONTROL2);
	out_be32(regs + DDR_OFF(SDRAM_MODE), CFG_SYS_DDR_MODE_1);
	out_be32(regs + DDR_OFF(SDRAM_MODE_2), CFG_SYS_DDR_MODE_2);
	out_be32(regs + DDR_OFF(SDRAM_MD_CNTL), CFG_SYS_MD_CNTL);
	out_be32(regs + DDR_OFF(ZQ_CNTL), CFG_SYS_DDR_ZQ_CONTROL);
	out_be32(regs + DDR_OFF(SR_CNTL), CFG_SYS_SR_CNTL);
	out_be32(regs + DDR_OFF(WRLVL_CNTL), CFG_SYS_DDR_WRLVL_CONTROL);
	out_be32(regs + DDR_OFF(WRLVL_CNTL_2), CFG_SYS_DDR_WRLVL_CONTROL_2);
	out_be32(regs + DDR_OFF(WRLVL_CNTL_3), CFG_SYS_DDR_WRLVL_CONTROL_3);

	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_3), CFG_SYS_DDR_SDRAM_MODE_3);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_4), CFG_SYS_DDR_SDRAM_MODE_4);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_5), CFG_SYS_DDR_SDRAM_MODE_5);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_6), CFG_SYS_DDR_SDRAM_MODE_6);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_7), CFG_SYS_DDR_SDRAM_MODE_7);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_8), CFG_SYS_DDR_SDRAM_MODE_8);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_9), CFG_SYS_DDR_SDRAM_MODE_9);
	out_be32(regs + DDR_OFF(SDRAM_MODE_CNTL_10), CFG_SYS_DDR_SDRAM_MODE_10);

	out_be32(regs + DDR_OFF(DDRCDR1), CFG_SYS_DDR_DDRCDR1);
	out_be32(regs + DDR_OFF(DDRCDR2), CFG_SYS_DDR_DDRCDR2);

	/* Basic refresh rate (7.8us),high temp is 3.9us  */
	out_be32(regs + DDR_OFF(SDRAM_INTERVAL),
		 CFG_SYS_DDR_INTERVAL);
	out_be32(regs + DDR_OFF(SDRAM_DATA_INIT),
		 CFG_SYS_DDR_DATA_INIT);
	out_be32(regs + DDR_OFF(SDRAM_CLK_CNTL),
		 CFG_SYS_DDR_CLK_CTRL);

	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR), 0);
	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR_EXT), 0);

	errata_A009942(regs);

	errata_A009988(regs);

	/* wait for DRAM data initialization */


	/*
	 * Wait 5ms for the DDR clock to stabilize.
	 */
	early_udelay(5000);
	asm volatile ("sync;isync");

	while (1) {
		early_udelay(2000);
		if (in_be32(regs + DDR_OFF(SDRAM_CFG_2)) & 0x00000010)
			break;
	}

	dram_size = fsl_get_effective_memsize();
	if (fsl_set_ddr_laws(0, dram_size, LAW_TRGT_IF_DDR) < 0)
		return 0;

	return dram_size;
}

static void invalidate_cpc(void)
{
    cpc_corenet_t *cpc = (cpc_corenet_t *)MPC85xx_CPC_ADDR;

    out_be32(&cpc->cpccsr0, CPC_CSR0_FI | CPC_CSR0_LFC);
    while (in_be32(&cpc->cpccsr0) & (CPC_CSR0_FI | CPC_CSR0_LFC))
        ;
}

static void fsl_enable_cpc(void)
{
    cpc_corenet_t *cpc = (cpc_corenet_t *)MPC85xx_CPC_ADDR;

    /* Enable CPC */
    out_be32(&cpc->cpccsr0, CPC_CSR0_CE | CPC_CSR0_PE);
    /* Read back to sync write */
    in_be32(&cpc->cpccsr0);

    invalidate_cpc();
}

static inline void fsl_set_ifc_cspr(int cs, u32 cspr)
{
	void __iomem *ifc = (void __iomem *)MPC85xx_IFC_ADDR;

	out_be32(ifc + CSPR_OFFSET(cs), cspr);
}

static int board_init_r(void)
{
	void __iomem *ifc_regs = (void __iomem *)(MPC85xx_IFC_ADDR);
	u32 csor;
	int reg;

	const u8 flash_low  = e500_find_tlb_idx((void *)CFG_FLASH_LOW,  1);
	const u8 flash_high = e500_find_tlb_idx((void *)CFG_FLASH_HIGH, 1);

#ifndef CONFIG_ENABLE_36BIT_PHYS
	const u8 bman_portal_low = e500_find_tlb_idx((void *)0xf4000000, 1);
	const u8 bman_portal_high = e500_find_tlb_idx((void *)0xf5000000, 1);
	const u8 qman_portal_low = e500_find_tlb_idx((void *)0xf6000000, 1);
	const u8 qman_portal_high = e500_find_tlb_idx((void *)0xf7000000, 1);
#endif

	int i = 0;
	pr_info("RCW: ");
	for (; i < 15; i++) {
		int *rcw = (int *) 0xfe0e0100 + (4 * i);
		pr_info("%x", *rcw);
	}
	pr_info("\n");

	/* Configure NOR on CS0 */
	fsl_set_ifc_cspr(0, (CSPR_BA & 0xe8000000) | CSPR_PS_16 | CSPR_TE | CSPR_MSEL_NOR | CSPR_V);

	/* Tune NOR timings */
	csor = in_be32(ifc_regs + MPC85xx_IFC_CSOR_OFFSET(0));
	csor &= ~(7 << 2);
	out_be32(ifc_regs + MPC85xx_IFC_CSOR_OFFSET(0), csor);
	out_be32(ifc_regs + MPC85xx_IFC_FTIM0_OFFSET(0), 0x10010101);
	/* Change timing to support 120ns memories.*/
	out_be32(ifc_regs + MPC85xx_IFC_FTIM1_OFFSET(0), 0x11000706);

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash */
	e500_disable_tlb(flash_low);
	e500_disable_tlb(flash_high);

#ifndef CONFIG_ENABLE_36BIT_PHYS
	e500_disable_tlb(bman_portal_low);
	e500_disable_tlb(bman_portal_high);
	e500_disable_tlb(qman_portal_low);
	e500_disable_tlb(qman_portal_high);
#endif

	/*
	 * Remap Boot flash region to caching-inhibited
	 * so that flash can be erased properly.
	 */
	e500_set_tlb(1, CFG_FLASH_LOW, CFG_FLASH_PHYS_LOW,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, flash_low, BOOKE_PAGESZ_64M, 1);

	e500_set_tlb(1, CFG_FLASH_HIGH, CFG_FLASH_PHYS_HIGH,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, flash_high, BOOKE_PAGESZ_64M, 1);

#ifndef CONFIG_ENABLE_36BIT_PHYS
	e500_set_tlb(1, 0xf4000000, 0xf4000000,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, bman_portal_low, BOOKE_PAGESZ_16M, 1);

	e500_set_tlb(1, 0xf5000000, 0xf5000000,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, bman_portal_high, BOOKE_PAGESZ_16M, 1);

	e500_set_tlb(1, 0xf6000000, 0xf6000000,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, qman_portal_low, BOOKE_PAGESZ_16M, 1);

	e500_set_tlb(1, 0xf7000000, 0xf7000000,
		     MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		     0, qman_portal_high, BOOKE_PAGESZ_16M, 1);
#endif

    fsl_enable_cpc();

	set_liodns();
	setup_portals();

#define CONFIG_SYS_FSL_SCFG_IODSECR1_ADDR 0xfe0fcd00
#define IODSECR1_LVDD_VAL  (0x7 << 26)
#define IODSECR1_L1VDD_VAL (0x7 << 23)
	reg = in_be32((unsigned *)CONFIG_SYS_FSL_SCFG_IODSECR1_ADDR);
	reg |= IODSECR1_LVDD_VAL | IODSECR1_L1VDD_VAL;
	out_be32((unsigned *)CONFIG_SYS_FSL_SCFG_IODSECR1_ADDR, reg);

	return 0;
}
core_initcall(board_init_r);
