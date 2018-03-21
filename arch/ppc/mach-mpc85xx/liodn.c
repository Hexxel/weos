/*
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/wrapper.h>
#include <common.h>

#include <mach/fsl_sec.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/fsl_portals.h>
#include <asm/fsl_liodn.h>
#include <asm/fsl_fman.h>


int get_dpaa_liodn(enum fsl_dpaa_dev dpaa_dev, u32 *liodns, int liodn_offset)
{
	liodns[0] = liodn_bases[dpaa_dev].id[0] + liodn_offset;

	if (liodn_bases[dpaa_dev].num_ids == 2)
		liodns[1] = liodn_bases[dpaa_dev].id[1] + liodn_offset;

	return liodn_bases[dpaa_dev].num_ids;
}

#ifdef CONFIG_SYS_SRIO
static void set_srio_liodn(struct srio_liodn_id_table *tbl, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		unsigned long reg_off = tbl[i].reg_offset[0];
		out_be32((u32 *)reg_off, tbl[i].id[0]);

		if (tbl[i].num_ids == 2) {
			reg_off = tbl[i].reg_offset[1];
			out_be32((u32 *)reg_off, tbl[i].id[1]);
		}
	}
}
#endif

static void set_liodn(struct liodn_id_table *tbl, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		u32 liodn;
		if (tbl[i].num_ids == 2) {
			liodn = (tbl[i].id[0] << 16) | tbl[i].id[1];
		} else {
			liodn = tbl[i].id[0];
		}

		out_be32((volatile u32 *)(tbl[i].reg_offset), liodn);
	}
}

static void set_fman_liodn(struct fman_liodn_id_table *tbl, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		u32 liodn;
		if (tbl[i].num_ids == 2)
			liodn = (tbl[i].id[0] << 16) | tbl[i].id[1];
		else
			liodn = tbl[i].id[0];

		out_be32((volatile u32 *)(tbl[i].reg_offset), liodn);
	}
}
static void setup_sec_liodn_base(void)
{
	ccsr_sec_t *sec = (void *)CONFIG_SYS_FSL_SEC_ADDR;
	u32 base;

	if (!IS_E_PROCESSOR(get_svr()))
		return;

	/* QILCR[QSLOM] */
	out_be32(&sec->qilcr_ms, 0x3ff<<16);

	base = (liodn_bases[FSL_HW_PORTAL_SEC].id[0] << 16) |
		liodn_bases[FSL_HW_PORTAL_SEC].id[1];

	out_be32(&sec->qilcr_ls, base);
}

#ifdef CONFIG_SYS_DPAA_FMAN
static void setup_fman_liodn_base(enum fsl_dpaa_dev dev,
				  struct fman_liodn_id_table *tbl, int size)
{
	int i;
	ccsr_fman_t *fm;
	u32 base;
        pr_err("%s: Entering\n", __func__);
	switch(dev) {
	case FSL_HW_PORTAL_FMAN1:
		fm = (void *)CONFIG_SYS_FSL_FM1_ADDR;
		break;

#if (CONFIG_SYS_NUM_FMAN == 2)
	case FSL_HW_PORTAL_FMAN2:
		fm = (void *)CONFIG_SYS_FSL_FM2_ADDR;
		break;
#endif
	default:
		printf("Error: Invalid device type to %s\n", __FUNCTION__);
		return ;
	}

	base = (liodn_bases[dev].id[0] << 16) | liodn_bases[dev].id[0];

	pr_err("Addr: %p, Base: 0x%x\n", fm->fm_dma.fmdmplr, base);
	/* setup all bases the same */
	for (i = 0; i < 32; i++) {
		out_be32(&fm->fm_dma.fmdmplr[i], base);
	}

	/* update tbl to ... */
	for (i = 0; i < size; i++)
		tbl[i].id[0] += liodn_bases[dev].id[0];
}
#endif

static void setup_pme_liodn_base(void)
{
#ifdef CONFIG_SYS_DPAA_PME
	ccsr_pme_t *pme = (void *)CONFIG_SYS_FSL_CORENET_PME_ADDR;
	u32 base = (liodn_bases[FSL_HW_PORTAL_PME].id[0] << 16) |
			liodn_bases[FSL_HW_PORTAL_PME].id[1];

	out_be32(&pme->liodnbr, base);
#endif
}

#ifdef CONFIG_SYS_FSL_RAID_ENGINE
static void setup_raide_liodn_base(void)
{
	struct ccsr_raide *raide = (void *)CONFIG_SYS_FSL_RAID_ENGINE_ADDR;

	/* setup raid engine liodn base for data/desc ; both set to 47 */
	u32 base = (liodn_bases[FSL_HW_PORTAL_RAID_ENGINE].id[0] << 16) |
			liodn_bases[FSL_HW_PORTAL_RAID_ENGINE].id[0];

	out_be32(&raide->liodnbr, base);
}
#endif

#ifdef CONFIG_SYS_DPAA_RMAN
static void set_rman_liodn(struct liodn_id_table *tbl, int size)
{
	int i;
	struct ccsr_rman *rman = (void *)CONFIG_SYS_FSL_CORENET_RMAN_ADDR;

	for (i = 0; i < size; i++) {
		/* write the RMan block number */
		out_be32(&rman->mmitar, i);
		/* write the liodn offset corresponding to the block */
		out_be32((u32 *)(tbl[i].reg_offset), tbl[i].id[0]);
	}
}

static void setup_rman_liodn_base(struct liodn_id_table *tbl, int size)
{
	int i;
	struct ccsr_rman *rman = (void *)CONFIG_SYS_FSL_CORENET_RMAN_ADDR;
	u32 base = liodn_bases[FSL_HW_PORTAL_RMAN].id[0];

	out_be32(&rman->mmliodnbr, base);

	/* update liodn offset */
	for (i = 0; i < size; i++)
		tbl[i].id[0] += base;
}
#endif

void set_liodns(void)
{
	/* setup general liodn offsets */
	set_liodn(liodn_tbl, liodn_tbl_sz);

#ifdef CONFIG_SYS_SRIO
	/* setup SRIO port liodns */
	set_srio_liodn(srio_liodn_tbl, srio_liodn_tbl_sz);
#endif

	/* setup SEC block liodn bases & offsets if we have one */
	if (IS_E_PROCESSOR(get_svr())) {
		set_liodn(sec_liodn_tbl, sec_liodn_tbl_sz);
		setup_sec_liodn_base();
	}

	/* setup FMAN block(s) liodn bases & offsets if we have one */
#ifdef CONFIG_SYS_DPAA_FMAN
	set_fman_liodn(fman1_liodn_tbl, fman1_liodn_tbl_sz);
	setup_fman_liodn_base(FSL_HW_PORTAL_FMAN1, fman1_liodn_tbl,
				fman1_liodn_tbl_sz);

#if (CONFIG_SYS_NUM_FMAN == 2)
	set_fman_liodn(fman2_liodn_tbl, fman2_liodn_tbl_sz);
	setup_fman_liodn_base(FSL_HW_PORTAL_FMAN2, fman2_liodn_tbl,
				fman2_liodn_tbl_sz);
#endif
#endif
	/* setup PME liodn base */
	setup_pme_liodn_base();

#ifdef CONFIG_SYS_FSL_RAID_ENGINE
	/* raid engine ccr addr code for liodn */
	set_liodn(raide_liodn_tbl, raide_liodn_tbl_sz);
	setup_raide_liodn_base();
#endif

#ifdef CONFIG_SYS_DPAA_RMAN
	/* setup RMan liodn offsets */
	set_rman_liodn(rman_liodn_tbl, rman_liodn_tbl_sz);
	/* setup RMan liodn base */
	setup_rman_liodn_base(rman_liodn_tbl, rman_liodn_tbl_sz);
#endif
}
