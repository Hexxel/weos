/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
 * Copyright 2007-2011 Freescale Semiconductor, Inc.
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
 * Based on U-Boot arch/powerpc/cpu/mpc85xx/fdt.c and
 * common/fdt_support.c - version git-2b26201.
 */
#include <common.h>
#include <init.h>
#include <errno.h>
#include <environment.h>
#include <asm/processor.h>
#include <mach/clock.h>
#include <of.h>
#include <mach/immap_85xx.h>
#include <libfdt.h>

#define CONFIG_SYS_NUM_CPC 1

static inline void ft_fixup_l3cache(void *blob)
{
    u32 line_size, num_ways, size, num_sets;
    cpc_corenet_t *cpc = (void *)MPC85xx_CPC_ADDR;
    u32 cfg0 = in_be32(&cpc->cpccfg0);
    struct device_node *l3_node;

    size = CPC_CFG0_SZ_K(cfg0) * 1024 * CONFIG_SYS_NUM_CPC;
    num_ways = CPC_CFG0_NUM_WAYS(cfg0);
    line_size = CPC_CFG0_LINE_SZ(cfg0);
    num_sets = size / (line_size * num_ways);

    l3_node = of_find_compatible_node(blob, NULL, "fsl,t1023-l3-cache-controller");
    if (l3_node) {
        of_property_write_u32(l3_node, "cache-unified", 0);
        of_property_write_u32(l3_node, "cache-block-size", line_size);
        of_property_write_u32(l3_node, "cache-size", size);
        of_property_write_u32(l3_node, "cache-sets", num_sets);
        of_property_write_u32(l3_node, "cache-level", 3);
#ifdef CONFIG_SYS_CACHE_STASHING
        of_property_write_u32(l3_node, "cache-stash-id", 1);
#endif
    }
}

static inline void ft_fixup_l2cache(void *blob)
{
    u32 l2cfg0 = mfspr(SPRN_L2CFG0);
    u32 size, line_size, num_ways, num_sets;
    struct device_node *cpu_node, *l2_node;
    const u32 *prop;
#ifdef CONFIG_SYS_CACHE_STASHING
    const u32 *reg;
#endif

    size = (l2cfg0 & 0x3fff) * 64 * 1024;
    num_ways = ((l2cfg0 >> 14) & 0x1f) + 1;
    line_size = (((l2cfg0 >> 23) & 0x3) + 1) * 32;
    num_sets = size / (line_size * num_ways);

    cpu_node = of_find_node_by_type(blob, "cpu");
    while (cpu_node) {
        prop = of_get_property(cpu_node, "next-level-cache", NULL);
        if (prop == NULL) {
            goto next;
        }

        l2_node = of_find_node_by_phandle(5);
        if (l2_node == NULL) {
            goto next;
        }

#ifdef CONFIG_SYS_CACHE_STASHING
            reg = of_get_property(cpu_node, "reg", NULL);
            if (reg) {
                of_property_write_u32(l2_node, "cache-cache-stash-id", (*reg * 2) + 32 + 1);
            }
#endif
            of_property_write_u32(l2_node, "cache-unified", 0);
            of_property_write_u32(l2_node, "cache-cache-block-size", line_size);
            of_property_write_u32(l2_node, "cache-size", size);
            of_property_write_u32(l2_node, "cache-sets", num_sets);
            of_property_write_u32(l2_node, "cache-level", 2);
            of_property_write_u8_array(l2_node, "compatible", "cache", sizeof("cache"));

next:
        cpu_node = of_find_node_by_type(cpu_node, "cpu");
    }

    ft_fixup_l3cache(blob);
}

static inline void ft_fixup_cache(void *blob)
{
    struct device_node *node;
    struct device_node *platform_node;

    platform_node = of_find_compatible_node(blob, NULL, "fsl,T1024RDB");
    if (platform_node == NULL)
        return;

    node = of_find_node_by_type(blob, "cpu");
    while (node) {
        u32 l1cfg0 = mfspr(SPRN_L1CFG0);
        u32 l1cfg1 = mfspr(SPRN_L1CFG1);
        u32 isize, iline_size, inum_sets, inum_ways;
        u32 dsize, dline_size, dnum_sets, dnum_ways;

        /* d-side config */
        dsize = (l1cfg0 & 0x7ff) * 1024;
        dnum_ways = ((l1cfg0 >> 11) & 0xff) + 1;
        dline_size = (((l1cfg0 >> 23) & 0x3) + 1) * 32;
        dnum_sets = dsize / (dline_size * dnum_ways);

        of_property_write_u32(node, "d-cache-block-size", dline_size);
        of_property_write_u32(node, "d-cache-size", dsize);
        of_property_write_u32(node, "d-cache-sets", dnum_sets);

        /* i-side config */
        isize = (l1cfg1 & 0x7ff) * 1024;
        inum_ways = ((l1cfg1 >> 11) & 0xff) + 1;
        iline_size = (((l1cfg1 >> 23) & 0x3) + 1) * 32;
        inum_sets = isize / (iline_size * inum_ways);

        of_property_write_u32(node, "i-cache-block-size", iline_size);
        of_property_write_u32(node, "i-cache-size", isize);
        of_property_write_u32(node, "i-cache-sets", inum_sets);

        node = of_find_node_by_type(node, "cpu");
    }

    ft_fixup_l2cache(blob);
}

static void of_setup_crypto_node(void *blob)
{
	struct device_node *crypto_node;

	crypto_node = of_find_compatible_node(blob, NULL, "fsl,sec2.0");
	if (crypto_node == NULL)
		return;

	of_delete_node(crypto_node);
}

/* These properties specify whether the hardware supports the stashing
 * of buffer descriptors in L2 cache.
 */
static void fdt_add_enet_stashing(void *fdt)
{
	struct device_node *node;

	node = of_find_compatible_node(fdt, NULL, "gianfar");
	while (node) {
		of_set_property(node, "bd-stash", NULL, 0, 1);
		of_property_write_u32(node, "rx-stash-len", 96);
		of_property_write_u32(node, "rx-stash-idx", 0);
		node = of_find_compatible_node(node, NULL, "gianfar");
	}

	node = of_find_compatible_node(fdt, NULL, "fsl,etsec2");
	while (node) {
		of_set_property(node, "bd-stash", NULL, 0, 1);
		of_property_write_u32(node, "rx-stash-len", 96);
		of_property_write_u32(node, "rx-stash-idx", 0);
		node = of_find_compatible_node(node, NULL, "fsl,etsec2");
	}
}

static int fdt_stdout_setup(struct device_node *blob)
{
	struct device_node *node, *alias;
	char sername[9] = { 0 };
	const char *prop;
	struct console_device *cdev;
	int len;

	node = of_create_node(blob, "/chosen");
	if (node == NULL) {
		pr_err("%s: could not open /chosen node\n", __func__);
		goto error;
	}

	cdev = console_get_first_active();
	if (cdev)
		sprintf(sername, "serial%d", cdev->dev->id);
	else
		sprintf(sername, "serial%d", 0);

	alias = of_find_node_by_path_from(blob, "/aliases");
	if (!alias) {
		pr_err("%s: could not get aliases node.\n", __func__);
		goto error;
	}
	prop = of_get_property(alias, sername, &len);
	of_set_property(node, "linux,stdout-path", prop, len, 1);

	return 0;
error:
	return -ENODEV;
}

static int fdt_cpu_setup(struct device_node *blob, void *unused)
{
	struct device_node *node;
	struct sys_info sysinfo;

	of_set_root_node(blob);

	/* delete crypto node if not on an E-processor */
	if (!IS_E_PROCESSOR(get_svr()))
		of_setup_crypto_node(blob);

    ft_fixup_cache(blob);

	fdt_add_enet_stashing(blob);
	fsl_get_sys_info(&sysinfo);

	node = of_find_node_by_type(blob, "cpu");
	while (node) {
		const uint32_t *reg;

		of_property_write_u32(node, "timebase-frequency",
				fsl_get_timebase_clock());
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqSystemBus);
		reg = of_get_property(node, "reg", NULL);
		of_property_write_u32(node, "clock-frequency",
				sysinfo.freqProcessor[*reg]);
		node = of_find_node_by_type(node, "cpu");
	}

	node = of_find_node_by_type(blob, "soc");
	if (node)
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqSystemBus);

	node = of_find_compatible_node(blob, NULL, "fsl,elbc");
	if (node)
		of_property_write_u32(node, "bus-frequency",
				sysinfo.freqLocalBus);

	node = of_find_compatible_node(blob, NULL, "ns16550");
	while (node) {
		of_property_write_u32(node, "clock-frequency",
                                 fsl_get_uart_freq());
		node = of_find_compatible_node(node, NULL, "ns16550");
	}

	node = of_find_compatible_node(blob, NULL, "fsl,mpic");
	if (node)
		of_property_write_u32(node, "clock-frequency",
				sysinfo.freqSystemBus);

	fdt_stdout_setup(blob);

	return 0;
}

static int of_register_mpc85xx_fixup(void)
{
	return of_register_fixup(fdt_cpu_setup, NULL);
}
late_initcall(of_register_mpc85xx_fixup);
