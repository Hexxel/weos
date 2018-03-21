/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
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
 * Board specific DDR tuning.
 */

#include <common.h>
#include <mach/fsl_i2c.h>
#include <mach/immap_85xx.h>
#include <mach/clock.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>

void fsl_ddr_board_options(struct memctl_options_s *popts,
			   struct dimm_params_s *pdimm)
{
	pdimm->n_ranks = 1;
	pdimm->rank_density = 536870912u;
	pdimm->capacity = 536870912u;
	pdimm->primary_sdram_width = 32;
	pdimm->ec_sdram_width = 0;
	pdimm->registered_dimm = 0;
	pdimm->mirrored_dimm = 0;
	pdimm->n_row_addr = 14;
	pdimm->n_col_addr = 10;
	pdimm->n_banks_per_sdram_device = 8;
	pdimm->edc_config = EDC_ECC;
	pdimm->burst_lengths_bitmask = 0x0c;
	pdimm->tCKmin_X_ps = 1500;
	pdimm->caslat_X = 0x7e << 4;  /* 5;6;7;8;9;10 */
	pdimm->taa_ps = 13500;
	pdimm->tWR_ps = 15000;
	pdimm->tRCD_ps = 13500;
	pdimm->tRRD_ps = 6000;
	pdimm->tRP_ps = 13500;
	pdimm->tRAS_ps = 36000;
	pdimm->tRC_ps = 49500;
	pdimm->tRFC_ps = 160000;
	pdimm->tWTR_ps = 7500;
	pdimm->tRTP_ps = 7500;
	pdimm->refresh_rate_ps = 7800000;
	pdimm->tfaw_ps = 30000;

	strncpy(pdimm->mpart, "MT41J128M16_15E", sizeof(pdimm->mpart));
}
