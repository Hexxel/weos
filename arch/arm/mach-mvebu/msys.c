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

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <of.h>
#include <of_address.h>
#include <asm/memory.h>
#include <linux/mbus.h>
#include <linux/sizes.h>
#include <mach/msys-regs.h>
#include <mach/socid.h>


static const struct devfs_partition flash_map[] = {
	{
		.name   = "self0",
		.offset = 0,
		.size   = SZ_512K + SZ_256K,
		.flags  = DEVFS_PARTITION_FIXED,
	},
	{
		.name   = "etc",
		.offset = SZ_512K + SZ_256K,
		.size   = SZ_256K,
		.flags  = DEVFS_PARTITION_FIXED,
	},
	{ .name = NULL }
};



unsigned int whoAmI(void)
{
	u32 value;

	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 5   @ read CPUID reg\n" : "=r"(value) : : "memory");
	return (value & 0x7);
}


void msys_cpu_init(void)
{
	volatile u32 temp;
	u32 reg;
	
	/* enable access to CP10 and CP11 */
	temp = 0x00f00000;
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 2" :: "r" (temp));

#if 0
	/* enable FPU */
	if (1) {
		/* init and Enable FPU to Run Fast Mode */
		pr_info("FPU initialized to Run Fast Mode.\n");
		/* Enable */
		temp = FPEXC_ENABLE;
		fmxr(FPEXC, temp);
		/* Run Fast Mode */
		temp = fmrx(FPSCR);
		temp |= (FPSCR_DEFAULT_NAN | FPSCR_FLUSHTOZERO);
		fmxr(FPSCR, temp);
	} else {
		pr_info("FPU not initialized\n");
		/* Disable */
		temp = fmrx(FPEXC);
		temp &= ~FPEXC_ENABLE;
		fmxr(FPEXC, temp);
	}
#endif

	__asm__ __volatile__ ("mrc p15, 1, %0, c15, c1, 2" : "=r" (temp));
	temp |= (BIT(25) | BIT(27) | BIT(29) | BIT(30));
	/* removed BIT23 in order to enable fast LDR bypass */
	__asm__ __volatile__ ("mcr p15, 1, %0, c15, c1, 2\n" \
			      "mcr p15, 0, %0, c7, c5, 4" : : "r" (temp)); /*imb*/
	
	/* Multi-CPU managment */
	/* MP mode (AMP or SMP) */
	if (0) {
		/* Set AMP in Auxilary control register */
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 1" : "=r" (temp));
		temp &= ~BIT(5);
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 1\n" \
						"mcr p15, 0, %0, c7, c5, 4" : : "r" (temp)); /* imb */

		/* Set AMP in Auxiliary Funcional Modes Control register */
		__asm__ __volatile__ ("mrc p15, 1, %0, c15, c2, 0" : "=r" (temp));
		temp &= ~BIT(1);
		__asm__ __volatile__ ("mcr p15, 1, %0, c15, c2, 0\n" \
						"mcr p15, 0, %0, c7, c5, 4" : : "r" (temp)); /* imb */
	}
	else {
		/* Set SMP in Auxilary control register */
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 1" : "=r" (temp));
		temp |= BIT(5);
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 1\n" \
						"mcr p15, 0, %0, c7, c5, 4" : : "r" (temp)); /* imb */

		/* Set SMP in Auxiliary Funcional Modes Control register */
		__asm__ __volatile__ ("mrc p15, 1, %0, c15, c2, 0" : "=r" (temp));
		temp |= BIT(1);
		__asm__ __volatile__ ("mcr p15, 1, %0, c15, c2, 0\n" \
						"mcr p15, 0, %0, c7, c5, 4" : : "r" (temp)); /* imb */

		/* Enable CPU respond to coherency fabric requests */
		/* Assaf: Note must be enabled for IO coherency as well */
		reg = readl(MSYS_FABRIC_CTRL);
		reg |= BIT(24 + whoAmI());
		writel(reg, MSYS_FABRIC_CTRL);
		
		/* Configure all Cores to be in SMP Group 0 */
		reg = readl(MSYS_FABRIC_CONF);
		reg |= BIT((24 + whoAmI()));
		writel(reg, MSYS_FABRIC_CONF);

		/* In loader mode, set fabric regs for both CPUs.*/

		/* enable MonExt (???) */
		/* Configure Core 1 to be in SMP Group 0 */
		reg = readl(MSYS_FABRIC_CONF);
		reg |= BIT(25 + whoAmI());
		writel(reg, MSYS_FABRIC_CONF);

		/* Set number of CPUs=2 (for Linux) */
		reg = readl(MSYS_FABRIC_CONF);
		reg |= BIT(1);
		writel(reg, MSYS_FABRIC_CONF);
	}

#if 0
	/* Set L2C WT mode */
	temp = MV_REG_READ(CPU_L2_AUX_CTRL_REG) & ~CL2ACR_WB_WT_ATTR_MASK;
	if (1)
		temp |= CL2ACR_WB_WT_ATTR_WT;
	 /* Set L2 algorithm to semi_pLRU */
	temp &= ~CL2ACR_REP_STRGY_MASK;
	if (mvCtrlRevGet() == 1)
		temp |= CL2ACR_REP_STRGY_semiPLRU_MASK;
	else{
		temp |= CL2ACR_REP_STRGY_semiPLRU_WA_MASK;
		temp |= CL2_DUAL_EVICTION;
		temp |= CL2_PARITY_ENABLE;
		temp |= CL2_InvalEvicLineUCErr;
	}

	MV_REG_WRITE(CPU_L2_AUX_CTRL_REG, temp);

	/* enable L2C */
	temp = MV_REG_READ(CPU_L2_CTRL_REG);

	env = getenv("disL2Cache");
	if (!env || ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)))
		temp |= CL2CR_L2_EN_MASK;
	else
		temp &= ~CL2CR_L2_EN_MASK;

	MV_REG_WRITE(CPU_L2_CTRL_REG, temp);

	/* Configure L2 options if L2 exists */
	if (MV_REG_READ(CPU_L2_CTRL_REG) & CL2CR_L2_EN_MASK) {
		/* Read L2 Auxilary control register */
		temp = MV_REG_READ(CPU_L2_AUX_CTRL_REG);
		/* Clear fields */
		temp &= ~(CL2ACR_WB_WT_ATTR_MASK | CL2ACR_FORCE_WA_MASK);

		/* Set "Force write policy" field */
		env = getenv("L2forceWrPolicy");
		if ( env && ((strcmp(env, "WB") == 0) || (strcmp(env, "wb") == 0)) )
			temp |= CL2ACR_WB_WT_ATTR_WB;
		else if ( env && ((strcmp(env, "WT") == 0) || (strcmp(env, "wt") == 0)) )
			temp |= CL2ACR_WB_WT_ATTR_WT;
		else
			temp |= CL2ACR_WB_WT_ATTR_PAGE;

		/* Set "Force Write Allocate" field */
		env = getenv("L2forceWrAlloc");
		if ( env && ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)) )
			temp |= CL2ACR_FORCE_NO_WA;
		else if ( env && ((strcmp(env, "yes") == 0) || (strcmp(env, "Yes") == 0)) )
			temp |= CL2ACR_FORCE_WA;
		else
			temp |= CL2ACR_FORCE_WA_DISABLE;

		/* Set "ECC" */
		env = getenv("L2EccEnable");
		if (!env || ( (strcmp(env, "no") == 0) || (strcmp(env, "No") == 0) ) )
			temp &= ~CL2ACR_ECC_EN;
		else
			temp |= CL2ACR_ECC_EN;

		/* Set other L2 configurations */
		temp |= (CL2ACR_PARITY_EN | CL2ACR_INVAL_UCE_EN);

		/* Set L2 algorithm to semi_pLRU */
		temp &= ~CL2ACR_REP_STRGY_MASK;

		temp |= CL2ACR_REP_STRGY_semiPLRU_MASK;

		/* Write to L2 Auxilary control register */
		MV_REG_WRITE(CPU_L2_AUX_CTRL_REG, temp);

		env = getenv("L2SpeculativeRdEn");
		if (env && ((strcmp(env, "no") == 0) || (strcmp(env, "No") == 0)) )
			MV_REG_BIT_SET(0x20228, ((0x1 << 5)));
		else
			MV_REG_BIT_RESET(0x20228, ((0x1 << 5)));

	}
#endif

	/* Enable i cache */
#if 0
	asm ("mrc p15, 0, %0, c1, c0, 0" : "=r" (temp));
	temp |= BIT12;
	/* Change reset vector to address 0x0 */
	temp &= ~BIT13;
	asm ("mcr p15, 0, %0, c1, c0, 0\n" \
	     "mcr p15, 0, %0, c7, c5, 4" : : "r" (temp));  /* imb */
#endif
}


static inline void msys_memory_find(unsigned long *phys_base,
					     unsigned long *phys_size)
{
	int cs;

	*phys_base = ~0;
	*phys_size = 0;

	for (cs = 0; cs < 4; cs++) {
		u32 base = readl(MSYS_SDRAM_BASE + DDR_BASE_CSn(cs));
		u32 ctrl = readl(MSYS_SDRAM_BASE + DDR_SIZE_CSn(cs));

		/* Skip non-enabled CS */
		if ((ctrl & DDR_SIZE_ENABLED) != DDR_SIZE_ENABLED)
			continue;

		base &= DDR_BASE_CS_LOW_MASK;
		if (base < *phys_base)
			*phys_base = base;
		*phys_size += (ctrl | ~DDR_SIZE_MASK) + 1;
	}
}

static const struct of_device_id msys_pcie_of_ids[] = {
	{ .compatible = "marvell,msys-pcie", },
	{ },
};


static int msys_soc_id_fixup(void)
{
	return 0;
}

static int msys_device_init(void)
{
	return devfs_create_partitions("m25p0", flash_map);
}
device_initcall(msys_device_init);

static void __noreturn msys_restart_soc(struct restart_handler *rst)
{
	writel(0x1, MSYS_SYSCTL_BASE + 0x60);
	writel(0x1, MSYS_SYSCTL_BASE + 0x64);
	hang();
}

static int msys_init_soc(struct device_node *root, void *context)
{
	unsigned long phys_base, phys_size;
	u32 reg;
   int ret;
	
	if (!of_machine_is_compatible("marvell,msys"))
		return 0;

	restart_handler_register_fn(msys_restart_soc);

	barebox_set_model("Marvell MSys");
	barebox_set_hostname("msys");
	
	/* Disable MBUS error propagation */
	reg = readl(MSYS_FABRIC_BASE);
	reg &= ~BIT(8);
	writel(reg, MSYS_FABRIC_BASE);

	msys_memory_find(&phys_base, &phys_size);
	ret = mvebu_set_memory(phys_base, phys_size);
   if (ret != 0)
      pr_err("Error mvebu_set_memory\n");
	ret = mvebu_mbus_init();
   if (ret != 0)
      pr_err("Error mvebu_mbus_init\n");

	msys_cpu_init();
	msys_soc_id_fixup();
		
	return 0;
}

static int msys_register_soc_fixup(void)
{
	mvebu_mbus_add_range("marvell,msys-mbus", 0xf0, 0x01,
			     MVEBU_REMAP_INT_REG_BASE);
	return of_register_fixup(msys_init_soc, NULL);
}
pure_initcall(msys_register_soc_fixup);
