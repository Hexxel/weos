/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc.
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

#include <common.h>
#include <asm/fsl_law.h>
#include <asm/mmu.h>

struct law_entry law_table[] = {
	FSL_SET_LAW(CFG_FLASH_BASE_PHYS, LAW_SIZE_128M, LAW_TRGT_IF_IFC),
	FSL_SET_LAW(CFG_BMAN_BASE_PHYS, LAW_SIZE_32M, LAW_TRGT_IF_BMAN),
	FSL_SET_LAW(CFG_QMAN_BASE_PHYS, LAW_SIZE_32M, LAW_TRGT_IF_QMAN),
	/* FSL_SET_LAW(CFG_FPGA_BASE_PHYS,  LAW_SIZE_64K,  LAW_TRGT_IF_LBC), */
};

int num_law_entries = ARRAY_SIZE(law_table);
