/*
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __ASM_PPC_FSL_IFC_H
#define __ASM_PPC_FSL_IFC_H

#include <config.h>
#include <common.h>

#include <mach/immap_85xx.h>

#define CSPR_OFFSET(_cs) (0x10 + 0xc * (_cs))

#define CSPR_V		0x00000001
#define CSPR_MSEL_MASK	0x00000006
#define CSPR_MSEL_NOR	0x00000000
#define CSPR_MSEL_NAND	0x00000002
#define CSPR_MSEL_GPCM	0x00000004
#define CSPR_TE		0x00000010
#define CSPR_WP		0x00000040
#define CSPR_PS_MASK	0x00000180
#define CSPR_PS_8	0x00000080
#define CSPR_PS_16	0x00000100
#define CSPR_BA(_base)	((_base) & 0xffff0000)

static inline void fsl_set_ifc_cspr(int cs, u32 cspr)
{
	void __iomem *ifc = (void __iomem *)MPC85xx_IFC_ADDR;

	out_be32(ifc + CSPR_OFFSET(cs), cspr);
}

#endif /* __ASM_PPC_FSL_IFC_H */
