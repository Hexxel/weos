/*
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
 * Copyright 2014 Westermo Teleindustri AB
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <of.h>
#include <linux/sizes.h>

#include <asm/cache.h>
#include <asm/processor.h>
#include <asm/fsl_law.h>

#include <mach/early_udelay.h>
#include <mach/immap_85xx.h>
#include <mach/mmu.h>
#include <mach/mpc85xx.h>

#define BOOT_ENTRY_ADDR_UPPER	0
#define BOOT_ENTRY_ADDR_LOWER	1
#define BOOT_ENTRY_R3_UPPER	2
#define BOOT_ENTRY_R3_LOWER	3
#define BOOT_ENTRY_RESV		4
#define BOOT_ENTRY_PIR		5
#define BOOT_ENTRY_R6_UPPER	6
#define BOOT_ENTRY_R6_LOWER	7
#define NUM_BOOT_ENTRY		8
#define SIZE_BOOT_ENTRY		(NUM_BOOT_ENTRY * sizeof(u32))

static volatile void *reg_whoami = (volatile void *)
	(MPC8xxx_PIC_ADDR + MPC85xx_PIC_WHOAMI_OFFSET);
static volatile void *reg_bptr = (volatile void *)
	(MPC85xx_LOCAL_ADDR + MPC85xx_LOCAL_BPTR_OFFSET);


#ifdef CONFIG_E500MC
static volatile void *reg_brr = (volatile void *)
	(MPC85xx_GUTS_ADDR + MPC85xx_GUTS_BRR_OFFSET);
static volatile void *reg_pctbenr = (volatile void *)
	(MPC85xx_RCPM_ADDR + MPC85xx_RCPM_PCTBENR_OFFSET);

static void setup_bptr(ulong bootpg)
{

	out_be32(reg_bptr + 4, bootpg - CFG_SDRAM_BASE);
	out_be32(reg_bptr + 8, BIT(31) | (LAW_TRGT_IF_DDR << 20) | LAW_SIZE_4K);
}

static void timebase_disable(int cpu)
{
	/* disable time base at the platform */
	out_be32(reg_pctbenr, 1 << cpu);
}

static void timebase_enable(int cpu)
{
	u32 mask = (1 << fsl_cpu_numcores()) - 1;

	/* enable time base at the platform */
	out_be32(reg_pctbenr, 0);

	/* readback to sync write */
	in_be32(reg_pctbenr);

	mtspr(SPRN_TBWU, 0);
	mtspr(SPRN_TBWL, 0);

	out_be32(reg_pctbenr, mask);
}

static void release_cores(void)
{
	u32 brr, mask;

	mask = (1 << fsl_cpu_numcores()) - 1;
	brr = in_be32(reg_brr);
	brr |= mask;

	flush_dcache();
	invalidate_icache();
	out_be32(reg_brr, brr);
	asm("sync; isync; msync");
}

#else
static volatile void *reg_devdisr = (volatile void *)
	(MPC85xx_GUTS_ADDR + MPC85xx_GUTS_DEVDISR_OFFSET);
static volatile void *reg_eebpcr = (volatile void *)
	(MPC85xx_ECM_ADDR + MPC85xx_ECM_EEBPCR_OFFSET);

static void setup_bptr(ulong bootpg)
{
	out_be32(reg_bptr, 0x80000000 | (bootpg >> 12));
}

u32 devdisr;

static void timebase_disable(int cpu)
{
	/* disable time base at the platform */
	devdisr = in_be32(reg_devdisr);
	if (cpu)
		devdisr |= MPC85xx_DEVDISR_TB0;
	else
		devdisr |= MPC85xx_DEVDISR_TB1;
	out_be32(reg_devdisr, devdisr);
}

static void timebase_enable(int cpu)
{
	/* enable time base at the platform */
	if (cpu)
		devdisr |= MPC85xx_DEVDISR_TB1;
	else
		devdisr |= MPC85xx_DEVDISR_TB0;
	out_be32(reg_devdisr, devdisr);

	/* readback to sync write */
	in_be32(reg_devdisr);

	mtspr(SPRN_TBWU, 0);
	mtspr(SPRN_TBWL, 0);

	devdisr &= ~(MPC85xx_DEVDISR_TB0 | MPC85xx_DEVDISR_TB1);
	out_be32(reg_devdisr, devdisr);
}

static void release_cores(void)
{
	u32 bpcr, mask;

	mask = (1 << fsl_cpu_numcores()) - 1;
	bpcr = in_be32(reg_eebpcr);
	bpcr |= (mask << 24);

	flush_dcache();
	invalidate_icache();
	out_be32(reg_eebpcr, bpcr);
	asm("sync; isync; msync");

}
#endif


static u32 sec_bootpg(void)
{
	return CFG_SDRAM_BASE + fsl_get_effective_memsize() - SZ_4K;
}

ulong get_spin_phys_addr(void)
{
	extern ulong __secondary_start_page;
	extern ulong __spin_table;

	return (sec_bootpg() +
		(ulong)&__spin_table - (ulong)&__secondary_start_page);
}

ulong get_spin_virt_addr(void)
{
	extern ulong __secondary_start_page;
	extern ulong __spin_table;

	return (0xfffff000 +
		(ulong)&__spin_table - (ulong)&__secondary_start_page);
}

static void cpu_init_sec(ulong bootpg)
{
	u32 up, cpu_up_mask, whoami;
	u32 *table = (u32 *)get_spin_virt_addr();
	int timeout = 10;

	whoami = in_be32(reg_whoami);

	setup_bptr(sec_bootpg());

	timebase_disable(whoami);

	release_cores();

	up = ((1 << fsl_cpu_numcores()) - 1);
	cpu_up_mask = 1 << whoami;

	/* wait for everyone */
	while (timeout) {
		int i;
		for (i = 0; i < fsl_cpu_numcores(); i++) {
			if (table[i * NUM_BOOT_ENTRY + BOOT_ENTRY_ADDR_LOWER])
				cpu_up_mask |= (1 << i);
		};

		if ((cpu_up_mask & up) == up)
			break;

		early_udelay(100);
		timeout--;
	}

	if (timeout == 0)
		printf("CPU up timeout. CPU up mask is %x should be %x\n",
			cpu_up_mask, up);

	timebase_enable(whoami);
}

int cpu_init_smp(void)
{
	extern ulong __secondary_start_page;
	extern ulong __bootpg_addr;

	__bootpg_addr = sec_bootpg();

	e500_set_tlb(1, 0xfffff000, sec_bootpg(), MAS3_SX|MAS3_SW|MAS3_SR,
		     MAS2_I|MAS2_G, 0, 0, BOOKE_PAGESZ_4K, 1);

	memcpy((void *)sec_bootpg(), &__secondary_start_page, SZ_4K);

	cpu_init_sec(sec_bootpg());

#ifdef CONFIG_BACKSIDE_L2_CACHE
	fsl_l2_cache_init();
#endif

	return 0;
}
device_initcall(cpu_init_smp);

static int cpu_fixup_smp(struct device_node *blob, void *unused)
{
	struct device_node *node;

	node = of_find_node_by_type(blob, "cpu");
	while (node) {
		const uint32_t *reg;

		of_property_write_u8_array(node, "enable-method", "spin-table",
					   sizeof("spin-table"));

		reg = of_get_property(node, "reg", NULL);

		of_property_write_u64(node, "cpu-release-addr",
				      *reg * SIZE_BOOT_ENTRY +
				      get_spin_phys_addr());

		node = of_find_node_by_type(node, "cpu");
	}

	of_add_reserve_entry(sec_bootpg(), SZ_4K);
	return 0;
}

static int cpu_smp_fixup_init(void)
{
	return of_register_fixup(cpu_fixup_smp, NULL);
}
late_initcall(cpu_smp_fixup_init);
