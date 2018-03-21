/*
 * Copyright 2011 Freescale Semiconductor, Inc.
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
#include <mach/mmu.h>

struct fsl_e_tlb_entry tlb_table[] = {
	/* TLB 0 - for temp stack in cache */
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR, CFG_INIT_RAM_ADDR,
			MAS3_SX|MAS3_SW|MAS3_SR, 0,
			0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (4 * 1024),
			CFG_INIT_RAM_ADDR + (4 * 1024),
			MAS3_SX|MAS3_SW|MAS3_SR, 0,
			0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (8 * 1024),
			CFG_INIT_RAM_ADDR + (8 * 1024),
			MAS3_SX|MAS3_SW|MAS3_SR, 0,
			0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (12 * 1024),
			CFG_INIT_RAM_ADDR + (12 * 1024),
			MAS3_SX|MAS3_SW|MAS3_SR, 0,
			0, 0, BOOKE_PAGESZ_4K, 0),

	/* TLB 1 */
	/* *I*** - Covers boot page */
	FSL_SET_TLB_ENTRY(1, 0xfffff000, 0xfffff000,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, 0, BOOKE_PAGESZ_4K, 1),

	/* *I*G* - CCSRBAR */
	FSL_SET_TLB_ENTRY(1, CFG_CCSRBAR, CFG_CCSRBAR_PHYS,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, 1, BOOKE_PAGESZ_16M, 1),

	/* W**G* - Flash/promjet, localbus */
	/* This will be changed to *I*G* after relocation to RAM. */
   FSL_SET_TLB_ENTRY(1, CFG_FLASH_LOW, CFG_FLASH_PHYS_LOW,
           MAS3_SX|MAS3_SR, MAS2_W|MAS2_G,
           0, 2, BOOKE_PAGESZ_64M, 1),

   FSL_SET_TLB_ENTRY(1, CFG_FLASH_HIGH, CFG_FLASH_PHYS_HIGH,
           MAS3_SX|MAS3_SR, MAS2_W|MAS2_G,
           0, 3, BOOKE_PAGESZ_64M, 1),

#if 1
	/* BMAN/QMAN Portals */
        FSL_SET_TLB_ENTRY(1, CFG_BMAN_BASE_PHYS, CFG_BMAN_BASE_PHYS,
                          MAS3_SX|MAS3_SW|MAS3_SR, 0,
                          0, 5, BOOKE_PAGESZ_16M, 1),
	FSL_SET_TLB_ENTRY(1, CFG_BMAN_BASE_PHYS + 0x01000000,
                          CFG_BMAN_BASE_PHYS + 0x01000000,
                          MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
                          0, 6, BOOKE_PAGESZ_16M, 1),
  	FSL_SET_TLB_ENTRY(1, CFG_QMAN_BASE_PHYS, CFG_QMAN_BASE_PHYS,
                          MAS3_SX|MAS3_SW|MAS3_SR, 0,
                          0, 7, BOOKE_PAGESZ_16M, 1),
	FSL_SET_TLB_ENTRY(1, CFG_QMAN_BASE_PHYS + 0x01000000,
                          CFG_QMAN_BASE_PHYS + 0x01000000,
                          MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
                          0, 8, BOOKE_PAGESZ_16M, 1),
#endif
};

int num_tlb_entries = ARRAY_SIZE(tlb_table);
