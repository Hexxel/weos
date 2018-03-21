/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
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
#include <asm/config.h>
#include <asm/fsl_law.h>
#include <mach/ffs64.h>

#define FSL_HW_NUM_LAWS FSL_NUM_LAWS

#ifdef FSL_LAW_EXT
#define LAW_BASE (CFG_IMMR + 0xc00)
#define LAWBARH_ADDR(x)    ((u32 *)LAW_BASE + 4*(x) + 0)
#define LAWBARL_ADDR(x)    ((u32 *)LAW_BASE + 4*(x) + 1)
#define LAWAR_ADDR(x)      ((u32 *)LAW_BASE + 4*(x) + 2)
#define LAWBAR_SHIFT 0
#else
#define LAW_BASE (CFG_IMMR + 0xc08)
#define LAWAR_ADDR(x) ((u32 *)LAW_BASE + 8 * (x) + 2)
#define LAWBAR_ADDR(x) ((u32 *)LAW_BASE + 8 * (x))
#define LAWBAR_SHIFT 12
#endif

/* CORONET#0>md 0xfe000c00
0_fe000c00 : 0000000f e8000000 81f0001b 00000000  ................
0_fe000c10 : 0000000f f4000000 81800018 00000000  ................
0_fe000c20 : 0000000f f6000000 83c00018 00000000  ................
0_fe000c30 : 0000000f ffdf0000 81f0000b 00000000  ................
*/

static inline phys_addr_t fsl_get_law_base_addr(int idx)
{
#ifdef CONFIG_ENABLE_36BIT_PHYS
	return (phys_addr_t)
		((u64)in_be32(LAWBARH_ADDR(idx)) << 32) |
		in_be32(LAWBARL_ADDR(idx));
#else
	return (phys_addr_t)in_be32(LAWBAR_ADDR(idx)) << LAWBAR_SHIFT;
#endif
}

static inline void fsl_set_law_base_addr(int idx, phys_addr_t addr)
{
#ifdef CONFIG_ENABLE_36BIT_PHYS
	out_be32(LAWBARL_ADDR(idx), addr & 0xffffffff);
	out_be32(LAWBARH_ADDR(idx), (u64)addr >> 32);
#else
	out_be32(LAWBAR_ADDR(idx), addr >> LAWBAR_SHIFT);
#endif
}

static void fsl_set_law(u8 idx, phys_addr_t addr, enum law_size sz,
		    enum law_trgt_if id)
{
	out_be32(LAWAR_ADDR(idx), 0);
	fsl_set_law_base_addr(idx, addr);
	out_be32(LAWAR_ADDR(idx), LAW_EN | ((u32)id << 20) | (u32)sz);

	/* Read back so that we sync the writes */
	in_be32(LAWAR_ADDR(idx));
}

static int fsl_is_free_law(int idx)
{
	u32 lawar;

	lawar = in_be32(LAWAR_ADDR(idx));
	if (!(lawar & LAW_EN))
		return 1;

	return 0;
}

static void fsl_set_next_law(phys_addr_t addr, enum law_size sz,
			enum law_trgt_if id)
{
	u32 idx;

	for (idx = 0; idx < FSL_HW_NUM_LAWS; idx++) {
		if (fsl_is_free_law(idx)) {
			fsl_set_law(idx, addr, sz, id);
			break;
		}
	}

	if (idx >= FSL_HW_NUM_LAWS)
		panic("No more LAWS available\n");
}

static void fsl_set_last_law(phys_addr_t addr, enum law_size sz,
			enum law_trgt_if id)
{
	u32 idx;

	for (idx = (FSL_HW_NUM_LAWS - 1); idx >= 0; idx--) {
		if (fsl_is_free_law(idx)) {
			fsl_set_law(idx, addr, sz, id);
			break;
		}
	}

	if (idx < 0)
		panic("No more LAWS available\n");
}

/* use up to 2 LAWs for DDR, use the last available LAWs */
int fsl_set_ddr_laws(u64 start, u64 sz, enum law_trgt_if id)
{
	u64 start_align, law_sz;
	int law_sz_enc;

	if (start == 0)
		start_align = 1ull << (LAW_SIZE_32G + 1);
	else
		start_align = 1ull << (ffs64(start) - 1);

	law_sz = min(start_align, sz);
	law_sz_enc = __ilog2_u64(law_sz) - 1;

	fsl_set_last_law(start, law_sz_enc, id);

	/* recalculate size based on what was actually covered by the law */
	law_sz = 1ull << __ilog2_u64(law_sz);

	/* do we still have anything to map */
	sz = sz - law_sz;
	if (sz) {
		start += law_sz;

		start_align = 1ull << (ffs64(start) - 1);
		law_sz = min(start_align, sz);
		law_sz_enc = __ilog2_u64(law_sz) - 1;

		fsl_set_last_law(start, law_sz_enc, id);
	} else {
		return 0;
	}

	/* do we still have anything to map */
	sz = sz - law_sz;
	if (sz)
		return 1;

	return 0;
}

void fsl_init_laws(void)
{
	int i;

	if (FSL_HW_NUM_LAWS > 32)
		panic("FSL_HW_NUM_LAWS can not be > 32 w/o code changes");

	for (i = 0; i < num_law_entries; i++) {
		if (law_table[i].index == -1)
			fsl_set_next_law(law_table[i].addr,
					law_table[i].size,
					law_table[i].trgt_id);
		else
			fsl_set_law(law_table[i].index, law_table[i].addr,
				law_table[i].size, law_table[i].trgt_id);
	}
}
