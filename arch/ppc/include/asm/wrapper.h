#ifndef _WRAPPER_H_
#define _WRAPPER_H_

#include <mach/immap_85xx.h>
#define CONFIG_SYS_CCSRBAR           CFG_CCSRBAR
#define CONFIG_SYS_CCSRBAR_PHYS      CFG_CCSRBAR_PHYS

#define CONFIG_SYS_DPAA_FMAN
#define CONFIG_SYS_FSL_SEC_ADDR      MPC85xx_SEC_ADDR
#define CONFIG_SYS_FSL_FM1_ADDR      MPC85xx_FMAN_FM1_ADDR
#define CONFIG_SYS_NUM_FMAN          1

#define CONFIG_SYS_DPAA_QBMAN
#define CONFIG_SYS_FSL_QMAN_V3
#define CONFIG_FSL_CORENET
#define CONFIG_SYS_FSL_QORIQ_CHASSIS2

#define CONFIG_SYS_FSL_FM1_OFFSET    0x400000

#define CONFIG_SYS_FSL_BMAN_ADDR     MPC85xx_BMAN_ADDR
#define CONFIG_SYS_FSL_BMAN_OFFSET   MPC85xx_BMAN_OFFSET

#define CONFIG_SYS_BMAN_SWP_ISDR_REG CFG_BMAN_SWP_ISDR_REG
#define CONFIG_SYS_BMAN_CINH_BASE    CFG_BMAN_CINH_BASE
#define CONFIG_SYS_BMAN_CINH_SIZE    CFG_BMAN_CINH_SIZE
#define CONFIG_SYS_BMAN_SP_CINH_SIZE CFG_BMAN_SP_CINH_SIZE
#define CONFIG_SYS_BMAN_NUM_PORTALS  CFG_BMAN_NUM_PORTALS
#define CONFIG_SYS_BMAN_MEM_PHYS     CFG_BMAN_BASE_PHYS

#define CONFIG_SYS_FSL_QMAN_ADDR     MPC85xx_QMAN_ADDR
#define CONFIG_SYS_FSL_QMAN_OFFSET   MPC85xx_QMAN_OFFSET
#define CONFIG_SYS_QMAN_SWP_ISDR_REG CFG_QMAN_SWP_ISDR_REG
#define CONFIG_SYS_QMAN_CINH_BASE    CFG_QMAN_CINH_BASE
#define CONFIG_SYS_QMAN_CINH_SIZE    CFG_QMAN_CINH_SIZE
#define CONFIG_SYS_QMAN_SP_CINH_SIZE CFG_QMAN_SP_CINH_SIZE
#define CONFIG_SYS_QMAN_NUM_PORTALS  CFG_QMAN_NUM_PORTALS
#define CONFIG_SYS_QMAN_MEM_PHYS     CFG_QMAN_BASE_PHYS

#define CONFIG_SYS_MPC85xx_DMA1_OFFSET MPC85xx_DMA1_OFFSET
#define CONFIG_SYS_MPC85xx_DMA2_OFFSET MPC85xx_DMA2_OFFSET
#define CONFIG_SYS_MPC85xx_USB1_OFFSET MPC85xx_USB1_OFFSET

#define CONFIG_SYS_FSL_FM1_RX0_1G_OFFSET	0x488000
#define CONFIG_SYS_FSL_FM1_RX1_1G_OFFSET	0x489000
#define CONFIG_SYS_FSL_FM1_RX2_1G_OFFSET	0x48a000
#define CONFIG_SYS_FSL_FM1_RX3_1G_OFFSET	0x48b000
#define CONFIG_SYS_FSL_FM1_RX4_1G_OFFSET	0x48c000
#define CONFIG_SYS_FSL_FM1_RX5_1G_OFFSET	0x48d000
#define CONFIG_SYS_FSL_FM1_RX0_10G_OFFSET	0x490000
#define CONFIG_SYS_FSL_FM1_RX1_10G_OFFSET	0x491000
#define CONFIG_SYS_FSL_FM1_DTSEC1_OFFSET	0x4e0000

#define CONFIG_SYS_FSL_SEC_OFFSET     MPC85xx_SEC_OFFSET
#define CONFIG_SYS_FSL_SEC_BE
#define CONFIG_SYS_FSL_SEC_COMPAT 5

/* GUTS */
#define CONFIG_SYS_FSL_QORIQ_CHASSIS2
#define CONFIG_PPC_T1024
#define CONFIG_SYS_FSL_SINGLE_SOURCE_CLK
#define CONFIG_SYS_MPC85xx_GUTS_OFFSET  MPC85xx_GUTS_OFFSET

typedef struct ccsr_qman {
#ifdef CONFIG_SYS_FSL_QMAN_V3
	u8	res0[0x200];
#else
	struct {
		u32	qcsp_lio_cfg;	/* 0x0 - SW Portal n LIO cfg */
		u32	qcsp_io_cfg;	/* 0x4 - SW Portal n IO cfg */
		u32	res;
		u32	qcsp_dd_cfg;	/* 0xc - SW Portal n Dynamic Debug cfg */
	} qcsp[32];
#endif
	/* Not actually reserved, but irrelevant to u-boot */
	u8	res[0xbf8 - 0x200];
	u32	ip_rev_1;
	u32	ip_rev_2;
	u32	fqd_bare;	/* FQD Extended Base Addr Register */
	u32	fqd_bar;	/* FQD Base Addr Register */
	u8	res1[0x8];
	u32	fqd_ar;		/* FQD Attributes Register */
	u8	res2[0xc];
	u32	pfdr_bare;	/* PFDR Extended Base Addr Register */
	u32	pfdr_bar;	/* PFDR Base Addr Register */
	u8	res3[0x8];
	u32	pfdr_ar;	/* PFDR Attributes Register */
	u8	res4[0x4c];
	u32	qcsp_bare;	/* QCSP Extended Base Addr Register */
	u32	qcsp_bar;	/* QCSP Base Addr Register */
	u8	res5[0x78];
	u32	ci_sched_cfg;	/* Initiator Scheduling Configuration */
	u32	srcidr;		/* Source ID Register */
	u32	liodnr;		/* LIODN Register */
	u8	res6[4];
	u32	ci_rlm_cfg;	/* Initiator Read Latency Monitor Cfg */
	u32	ci_rlm_avg;	/* Initiator Read Latency Monitor Avg */
	u8	res7[0x2e8];
#ifdef CONFIG_SYS_FSL_QMAN_V3
	struct {
		u32	qcsp_lio_cfg;	/* 0x0 - SW Portal n LIO cfg */
		u32	qcsp_io_cfg;	/* 0x4 - SW Portal n IO cfg */
		u32	res;
		u32	qcsp_dd_cfg;	/* 0xc - SW Portal n Dynamic Debug cfg*/
	} qcsp[50];
#endif
} ccsr_qman_t;

typedef struct ccsr_bman {
	/* Not actually reserved, but irrelevant to u-boot */
	u8	res[0xbf8];
	u32	ip_rev_1;
	u32	ip_rev_2;
	u32	fbpr_bare;	/* FBPR Extended Base Addr Register */
	u32	fbpr_bar;	/* FBPR Base Addr Register */
	u8	res1[0x8];
	u32	fbpr_ar;	/* FBPR Attributes Register */
	u8	res2[0xf0];
	u32	srcidr;		/* Source ID Register */
	u32	liodnr;		/* LIODN Register */
	u8	res7[0x2f4];
} ccsr_bman_t;

typedef struct ccsr_gur {
	u32	porsr1;		/* POR status 1 */
	u32	porsr2;		/* POR status 2 */
#ifdef	CONFIG_SYS_FSL_SINGLE_SOURCE_CLK
#define	FSL_DCFG_PORSR1_SYSCLK_SHIFT	15
#define	FSL_DCFG_PORSR1_SYSCLK_MASK	0x1
#define	FSL_DCFG_PORSR1_SYSCLK_SINGLE_ENDED	0x1
#define	FSL_DCFG_PORSR1_SYSCLK_DIFF	0x0
#endif
	u8	res_008[0x20-0x8];
	u32	gpporcr1;	/* General-purpose POR configuration */
	u32	gpporcr2;	/* General-purpose POR configuration 2 */
	u32	dcfg_fusesr;	/* Fuse status register */
#define FSL_CORENET_DCFG_FUSESR_VID_SHIFT	25
#define FSL_CORENET_DCFG_FUSESR_VID_MASK	0x1F
#define FSL_CORENET_DCFG_FUSESR_ALTVID_SHIFT	20
#define FSL_CORENET_DCFG_FUSESR_ALTVID_MASK	0x1F
	u8	res_02c[0x70-0x2c];
	u32	devdisr;	/* Device disable control */
	u32	devdisr2;	/* Device disable control 2 */
	u32	devdisr3;	/* Device disable control 3 */
	u32	devdisr4;	/* Device disable control 4 */
#ifdef CONFIG_SYS_FSL_QORIQ_CHASSIS2
	u32	devdisr5;	/* Device disable control 5 */
#define FSL_CORENET_DEVDISR_PBL	0x80000000
#define FSL_CORENET_DEVDISR_PMAN	0x40000000
#define FSL_CORENET_DEVDISR_ESDHC	0x20000000
#define FSL_CORENET_DEVDISR_DMA1	0x00800000
#define FSL_CORENET_DEVDISR_DMA2	0x00400000
#define FSL_CORENET_DEVDISR_USB1	0x00080000
#define FSL_CORENET_DEVDISR_USB2	0x00040000
#define FSL_CORENET_DEVDISR_SATA1	0x00008000
#define FSL_CORENET_DEVDISR_SATA2	0x00004000
#define FSL_CORENET_DEVDISR_PME	0x00000800
#define FSL_CORENET_DEVDISR_SEC	0x00000200
#define FSL_CORENET_DEVDISR_RMU	0x00000080
#define FSL_CORENET_DEVDISR_DCE	0x00000040
#define FSL_CORENET_DEVDISR2_DTSEC1_1	0x80000000
#define FSL_CORENET_DEVDISR2_DTSEC1_2	0x40000000
#define FSL_CORENET_DEVDISR2_DTSEC1_3	0x20000000
#define FSL_CORENET_DEVDISR2_DTSEC1_4	0x10000000
#define FSL_CORENET_DEVDISR2_DTSEC1_5	0x08000000
#define FSL_CORENET_DEVDISR2_DTSEC1_6	0x04000000
#define FSL_CORENET_DEVDISR2_DTSEC1_9	0x00800000
#define FSL_CORENET_DEVDISR2_DTSEC1_10	0x00400000
#ifdef CONFIG_FSL_FM_10GEC_REGULAR_NOTATION
#define FSL_CORENET_DEVDISR2_10GEC1_1   0x80000000
#define FSL_CORENET_DEVDISR2_10GEC1_2   0x40000000
#else
#define FSL_CORENET_DEVDISR2_10GEC1_1	0x00800000
#define FSL_CORENET_DEVDISR2_10GEC1_2	0x00400000
#define FSL_CORENET_DEVDISR2_10GEC1_3	0x80000000
#define FSL_CORENET_DEVDISR2_10GEC1_4	0x40000000
#endif
#define FSL_CORENET_DEVDISR2_DTSEC2_1	0x00080000
#define FSL_CORENET_DEVDISR2_DTSEC2_2	0x00040000
#define FSL_CORENET_DEVDISR2_DTSEC2_3	0x00020000
#define FSL_CORENET_DEVDISR2_DTSEC2_4	0x00010000
#define FSL_CORENET_DEVDISR2_DTSEC2_5	0x00008000
#define FSL_CORENET_DEVDISR2_DTSEC2_6	0x00004000
#define FSL_CORENET_DEVDISR2_DTSEC2_9	0x00000800
#define FSL_CORENET_DEVDISR2_DTSEC2_10	0x00000400
#define FSL_CORENET_DEVDISR2_10GEC2_1	0x00000800
#define FSL_CORENET_DEVDISR2_10GEC2_2	0x00000400
#define FSL_CORENET_DEVDISR2_FM1	0x00000080
#define FSL_CORENET_DEVDISR2_FM2	0x00000040
#define FSL_CORENET_DEVDISR2_CPRI	0x00000008
#define FSL_CORENET_DEVDISR3_PCIE1	0x80000000
#define FSL_CORENET_DEVDISR3_PCIE2	0x40000000
#define FSL_CORENET_DEVDISR3_PCIE3	0x20000000
#define FSL_CORENET_DEVDISR3_PCIE4	0x10000000
#define FSL_CORENET_DEVDISR3_SRIO1	0x08000000
#define FSL_CORENET_DEVDISR3_SRIO2	0x04000000
#define FSL_CORENET_DEVDISR3_QMAN	0x00080000
#define FSL_CORENET_DEVDISR3_BMAN	0x00040000
#define FSL_CORENET_DEVDISR3_LA1	0x00008000
#define FSL_CORENET_DEVDISR3_MAPLE1	0x00000800
#define FSL_CORENET_DEVDISR3_MAPLE2	0x00000400
#define FSL_CORENET_DEVDISR3_MAPLE3	0x00000200
#define FSL_CORENET_DEVDISR4_I2C1	0x80000000
#define FSL_CORENET_DEVDISR4_I2C2	0x40000000
#define FSL_CORENET_DEVDISR4_DUART1	0x20000000
#define FSL_CORENET_DEVDISR4_DUART2	0x10000000
#define FSL_CORENET_DEVDISR4_ESPI	0x08000000
#define FSL_CORENET_DEVDISR5_DDR1	0x80000000
#define FSL_CORENET_DEVDISR5_DDR2	0x40000000
#define FSL_CORENET_DEVDISR5_DDR3	0x20000000
#define FSL_CORENET_DEVDISR5_CPC1	0x08000000
#define FSL_CORENET_DEVDISR5_CPC2	0x04000000
#define FSL_CORENET_DEVDISR5_CPC3	0x02000000
#define FSL_CORENET_DEVDISR5_IFC	0x00800000
#define FSL_CORENET_DEVDISR5_GPIO	0x00400000
#define FSL_CORENET_DEVDISR5_DBG	0x00200000
#define FSL_CORENET_DEVDISR5_NAL	0x00100000
#define FSL_CORENET_DEVDISR5_TIMERS	0x00020000
#define FSL_CORENET_NUM_DEVDISR		5
#else
#define FSL_CORENET_DEVDISR_PCIE1	0x80000000
#define FSL_CORENET_DEVDISR_PCIE2	0x40000000
#define FSL_CORENET_DEVDISR_PCIE3	0x20000000
#define FSL_CORENET_DEVDISR_PCIE4	0x10000000
#define FSL_CORENET_DEVDISR_RMU		0x08000000
#define FSL_CORENET_DEVDISR_SRIO1	0x04000000
#define FSL_CORENET_DEVDISR_SRIO2	0x02000000
#define FSL_CORENET_DEVDISR_DMA1	0x00400000
#define FSL_CORENET_DEVDISR_DMA2	0x00200000
#define FSL_CORENET_DEVDISR_DDR1	0x00100000
#define FSL_CORENET_DEVDISR_DDR2	0x00080000
#define FSL_CORENET_DEVDISR_DBG		0x00010000
#define FSL_CORENET_DEVDISR_NAL		0x00008000
#define FSL_CORENET_DEVDISR_SATA1	0x00004000
#define FSL_CORENET_DEVDISR_SATA2	0x00002000
#define FSL_CORENET_DEVDISR_ELBC	0x00001000
#define FSL_CORENET_DEVDISR_USB1	0x00000800
#define FSL_CORENET_DEVDISR_USB2	0x00000400
#define FSL_CORENET_DEVDISR_ESDHC	0x00000100
#define FSL_CORENET_DEVDISR_GPIO	0x00000080
#define FSL_CORENET_DEVDISR_ESPI	0x00000040
#define FSL_CORENET_DEVDISR_I2C1	0x00000020
#define FSL_CORENET_DEVDISR_I2C2	0x00000010
#define FSL_CORENET_DEVDISR_DUART1	0x00000002
#define FSL_CORENET_DEVDISR_DUART2	0x00000001
#define FSL_CORENET_DEVDISR2_PME	0x80000000
#define FSL_CORENET_DEVDISR2_SEC	0x40000000
#define FSL_CORENET_DEVDISR2_QMBM	0x08000000
#define FSL_CORENET_DEVDISR2_FM1	0x02000000
#define FSL_CORENET_DEVDISR2_10GEC1	0x01000000
#define FSL_CORENET_DEVDISR2_DTSEC1_1	0x00800000
#define FSL_CORENET_DEVDISR2_DTSEC1_2	0x00400000
#define FSL_CORENET_DEVDISR2_DTSEC1_3	0x00200000
#define FSL_CORENET_DEVDISR2_DTSEC1_4	0x00100000
#define FSL_CORENET_DEVDISR2_DTSEC1_5	0x00080000
#define FSL_CORENET_DEVDISR2_FM2	0x00020000
#define FSL_CORENET_DEVDISR2_10GEC2	0x00010000
#define FSL_CORENET_DEVDISR2_DTSEC2_1	0x00008000
#define FSL_CORENET_DEVDISR2_DTSEC2_2	0x00004000
#define FSL_CORENET_DEVDISR2_DTSEC2_3	0x00002000
#define FSL_CORENET_DEVDISR2_DTSEC2_4	0x00001000
#define FSL_CORENET_DEVDISR2_DTSEC2_5	0x00000800
#define FSL_CORENET_NUM_DEVDISR		2
	u32	powmgtcsr;	/* Power management status & control */
#endif
	u8	res8[12];
	u32	coredisru;	/* uppper portion for support of 64 cores */
	u32	coredisrl;	/* lower portion for support of 64 cores */
	u8	res9[8];
	u32	pvr;		/* Processor version */
	u32	svr;		/* System version */
	u8	res10[8];
	u32	rstcr;		/* Reset control */
	u32	rstrqpblsr;	/* Reset request preboot loader status */
	u8	res11[8];
	u32	rstrqmr1;	/* Reset request mask */
#ifdef CONFIG_SYS_FSL_QORIQ_CHASSIS2
#define FSL_CORENET_RSTRQMR1_SRDS_RST_MSK      0x00000800
#endif
	u8	res12[4];
	u32	rstrqsr1;	/* Reset request status */
	u8	res13[4];
	u8	res14[4];
	u32	rstrqwdtmrl;	/* Reset request WDT mask */
	u8	res15[4];
	u32	rstrqwdtsrl;	/* Reset request WDT status */
	u8	res16[4];
	u32	brrl;		/* Boot release */
	u8	res17[24];
	u32	rcwsr[16];	/* Reset control word status */

#ifdef CONFIG_SYS_FSL_QORIQ_CHASSIS2
#define FSL_CORENET_RCWSR0_MEM_PLL_RAT_SHIFT	16
/* use reserved bits 18~23 as scratch space to host DDR PLL ratio */
#define FSL_CORENET_RCWSR0_MEM_PLL_RAT_RESV_SHIFT	8
#define FSL_CORENET_RCWSR0_MEM_PLL_RAT_MASK	0x3f
#if defined(CONFIG_PPC_T4240) || defined(CONFIG_PPC_T4160) || \
	defined(CONFIG_PPC_T4080)
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL		0xfc000000
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL_SHIFT	26
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL		0x00fe0000
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL_SHIFT	17
#define FSL_CORENET2_RCWSR4_SRDS3_PRTCL		0x0000f800
#define FSL_CORENET2_RCWSR4_SRDS3_PRTCL_SHIFT	11
#define FSL_CORENET2_RCWSR4_SRDS4_PRTCL		0x000000f8
#define FSL_CORENET2_RCWSR4_SRDS4_PRTCL_SHIFT	3
#define FSL_CORENET_RCWSR6_BOOT_LOC	0x0f800000
#elif defined(CONFIG_PPC_B4860) || defined(CONFIG_PPC_B4420)
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL	0xfe000000
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL_SHIFT	25
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL	0x00ff0000
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL_SHIFT	16
#define FSL_CORENET_RCWSR6_BOOT_LOC	0x0f800000
#elif defined(CONFIG_PPC_T1040) || defined(CONFIG_PPC_T1042) ||\
defined(CONFIG_PPC_T1020) || defined(CONFIG_PPC_T1022)
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL	0xff000000
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL_SHIFT	24
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL	0x00fe0000
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL_SHIFT	17
#define FSL_CORENET_RCWSR13_EC1	0x30000000 /* bits 418..419 */
#define FSL_CORENET_RCWSR13_EC1_FM1_DTSEC4_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC1_FM1_GPIO	0x10000000
#define FSL_CORENET_RCWSR13_EC1_FM1_DTSEC4_MII	0x20000000
#define FSL_CORENET_RCWSR13_EC2	0x0c000000 /* bits 420..421 */
#define FSL_CORENET_RCWSR13_EC2_FM1_DTSEC5_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC2_FM1_GPIO	0x10000000
#define FSL_CORENET_RCWSR13_EC2_FM1_DTSEC5_MII	0x20000000
#define FSL_CORENET_RCWSR13_MAC2_GMII_SEL	0x00000080
#define FSL_CORENET_RCWSR13_MAC2_GMII_SEL_L2_SWITCH	0x00000000
#define FSL_CORENET_RCWSR13_MAC2_GMII_SEL_ENET_PORT	0x80000000
#define CONFIG_SYS_FSL_SCFG_PIXCLKCR_OFFSET	0x28
#define PXCKEN_MASK	0x80000000
#define PXCK_MASK	0x00FF0000
#define PXCK_BITS_START	16
#elif defined(CONFIG_PPC_T1024) || defined(CONFIG_PPC_T1023) || \
	defined(CONFIG_PPC_T1014) || defined(CONFIG_PPC_T1013)
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL		0xff800000
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL_SHIFT	23
#define FSL_CORENET_RCWSR6_BOOT_LOC		0x0f800000
#define FSL_CORENET_RCWSR13_EC1			0x30000000 /* bits 418..419 */
#define FSL_CORENET_RCWSR13_EC1_RGMII		0x00000000
#define FSL_CORENET_RCWSR13_EC1_GPIO		0x10000000
#define FSL_CORENET_RCWSR13_EC2			0x0c000000
#define FSL_CORENET_RCWSR13_EC2_RGMII		0x08000000
#define CONFIG_SYS_FSL_SCFG_PIXCLKCR_OFFSET	0x28
#define CONFIG_SYS_FSL_SCFG_IODSECR1_OFFSET	0xd00
#define PXCKEN_MASK				0x80000000
#define PXCK_MASK				0x00FF0000
#define PXCK_BITS_START				16
#elif defined(CONFIG_PPC_T2080) || defined(CONFIG_PPC_T2081)
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL		0xff000000
#define FSL_CORENET2_RCWSR4_SRDS1_PRTCL_SHIFT	24
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL		0x00ff0000
#define FSL_CORENET2_RCWSR4_SRDS2_PRTCL_SHIFT	16
#define FSL_CORENET_RCWSR6_BOOT_LOC		0x0f800000
#endif
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S1_PLL1	0x00800000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S1_PLL2	0x00400000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S2_PLL1	0x00200000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S2_PLL2	0x00100000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S3_PLL1	0x00080000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S3_PLL2	0x00040000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S4_PLL1	0x00020000
#define FSL_CORENET2_RCWSR5_SRDS_PLL_PD_S4_PLL2	0x00010000
#define FSL_CORENET2_RCWSR5_DDR_REFCLK_SEL_SHIFT 4
#define FSL_CORENET2_RCWSR5_DDR_REFCLK_SEL_MASK	0x00000011
#define FSL_CORENET2_RCWSR5_DDR_REFCLK_SINGLE_CLK	1

#else /* CONFIG_SYS_FSL_QORIQ_CHASSIS2 */
#define FSL_CORENET_RCWSR0_MEM_PLL_RAT_SHIFT	17
#define FSL_CORENET_RCWSR0_MEM_PLL_RAT_MASK	0x1f
#define FSL_CORENET_RCWSR4_SRDS_PRTCL		0xfc000000
#define FSL_CORENET_RCWSR5_DDR_SYNC		0x00000080
#define FSL_CORENET_RCWSR5_DDR_SYNC_SHIFT		 7
#define FSL_CORENET_RCWSR5_SRDS_EN		0x00002000
#define FSL_CORENET_RCWSR5_SRDS2_EN		0x00001000
#define FSL_CORENET_RCWSR6_BOOT_LOC	0x0f800000
#define FSL_CORENET_RCWSRn_SRDS_LPD_B2		0x3c000000 /* bits 162..165 */
#define FSL_CORENET_RCWSRn_SRDS_LPD_B3		0x003c0000 /* bits 170..173 */
#endif /* CONFIG_SYS_FSL_QORIQ_CHASSIS2 */

#define FSL_CORENET_RCWSR7_MCK_TO_PLAT_RAT	0x00400000
#define FSL_CORENET_RCWSR8_HOST_AGT_B1		0x00e00000
#define FSL_CORENET_RCWSR8_HOST_AGT_B2		0x00100000
#define FSL_CORENET_RCWSR11_EC1			0x00c00000 /* bits 360..361 */
#ifdef CONFIG_PPC_P4080
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC1		0x00000000
#define FSL_CORENET_RCWSR11_EC1_FM1_USB1		0x00800000
#define FSL_CORENET_RCWSR11_EC2			0x001c0000 /* bits 363..365 */
#define FSL_CORENET_RCWSR11_EC2_FM2_DTSEC1		0x00000000
#define FSL_CORENET_RCWSR11_EC2_FM1_DTSEC2		0x00080000
#define FSL_CORENET_RCWSR11_EC2_USB2			0x00100000
#endif
#if defined(CONFIG_PPC_P2041) \
	|| defined(CONFIG_PPC_P3041) || defined(CONFIG_PPC_P5020)
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC4_RGMII	0x00000000
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC4_MII		0x00800000
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC4_NONE		0x00c00000
#define FSL_CORENET_RCWSR11_EC2			0x00180000 /* bits 363..364 */
#define FSL_CORENET_RCWSR11_EC2_FM1_DTSEC5_RGMII	0x00000000
#define FSL_CORENET_RCWSR11_EC2_FM1_DTSEC5_MII		0x00100000
#define FSL_CORENET_RCWSR11_EC2_FM1_DTSEC5_NONE		0x00180000
#endif
#if defined(CONFIG_PPC_P5040)
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC5_RGMII        0x00000000
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC5_MII          0x00800000
#define FSL_CORENET_RCWSR11_EC1_FM1_DTSEC5_NONE         0x00c00000
#define FSL_CORENET_RCWSR11_EC2                 0x00180000 /* bits 363..364 */
#define FSL_CORENET_RCWSR11_EC2_FM2_DTSEC5_RGMII        0x00000000
#define FSL_CORENET_RCWSR11_EC2_FM2_DTSEC5_MII          0x00100000
#define FSL_CORENET_RCWSR11_EC2_FM2_DTSEC5_NONE         0x00180000
#endif
#if defined(CONFIG_PPC_T4240) || defined(CONFIG_PPC_T4160) || \
	defined(CONFIG_PPC_T4080)
#define FSL_CORENET_RCWSR13_EC1			0x60000000 /* bits 417..418 */
#define FSL_CORENET_RCWSR13_EC1_FM2_DTSEC5_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC1_FM2_GPIO		0x40000000
#define FSL_CORENET_RCWSR13_EC2			0x18000000 /* bits 419..420 */
#define FSL_CORENET_RCWSR13_EC2_FM1_DTSEC5_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC2_FM1_DTSEC6_RGMII	0x08000000
#define FSL_CORENET_RCWSR13_EC2_FM1_GPIO		0x10000000
#endif
#if defined(CONFIG_PPC_T2080) || defined(CONFIG_PPC_T2081)
#define FSL_CORENET_RCWSR13_EC1			0x60000000 /* bits 417..418 */
#define FSL_CORENET_RCWSR13_EC1_DTSEC3_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC1_GPIO		0x40000000
#define FSL_CORENET_RCWSR13_EC2			0x18000000 /* bits 419..420 */
#define FSL_CORENET_RCWSR13_EC2_DTSEC4_RGMII	0x00000000
#define FSL_CORENET_RCWSR13_EC2_DTSEC10_RGMII	0x08000000
#define FSL_CORENET_RCWSR13_EC2_GPIO		0x10000000
#endif
	u8	res18[192];
	u32	scratchrw[4];	/* Scratch Read/Write */
	u8	res19[240];
	u32	scratchw1r[4];	/* Scratch Read (Write once) */
	u8	res20[240];
	u32	scrtsr[8];	/* Core reset status */
	u8	res21[224];
	u32	pex1liodnr;	/* PCI Express 1 LIODN */
	u32	pex2liodnr;	/* PCI Express 2 LIODN */
	u32	pex3liodnr;	/* PCI Express 3 LIODN */
	u32	pex4liodnr;	/* PCI Express 4 LIODN */
	u32	rio1liodnr;	/* RIO 1 LIODN */
	u32	rio2liodnr;	/* RIO 2 LIODN */
	u32	rio3liodnr;	/* RIO 3 LIODN */
	u32	rio4liodnr;	/* RIO 4 LIODN */
	u32	usb1liodnr;	/* USB 1 LIODN */
	u32	usb2liodnr;	/* USB 2 LIODN */
	u32	usb3liodnr;	/* USB 3 LIODN */
	u32	usb4liodnr;	/* USB 4 LIODN */
	u32	sdmmc1liodnr;	/* SD/MMC 1 LIODN */
	u32	sdmmc2liodnr;	/* SD/MMC 2 LIODN */
	u32	sdmmc3liodnr;	/* SD/MMC 3 LIODN */
	u32	sdmmc4liodnr;	/* SD/MMC 4 LIODN */
	u32	rio1maintliodnr;/* RIO 1 Maintenance LIODN */
	u32	rio2maintliodnr;/* RIO 2 Maintenance LIODN */
	u32	rio3maintliodnr;/* RIO 3 Maintenance LIODN */
	u32	rio4maintliodnr;/* RIO 4 Maintenance LIODN */
	u32	sata1liodnr;	/* SATA 1 LIODN */
	u32	sata2liodnr;	/* SATA 2 LIODN */
	u32	sata3liodnr;	/* SATA 3 LIODN */
	u32	sata4liodnr;	/* SATA 4 LIODN */
	u8	res22[20];
	u32	tdmliodnr;	/* TDM LIODN */
	u32     qeliodnr;       /* QE LIODN */
	u8      res_57c[4];
	u32	dma1liodnr;	/* DMA 1 LIODN */
	u32	dma2liodnr;	/* DMA 2 LIODN */
	u32	dma3liodnr;	/* DMA 3 LIODN */
	u32	dma4liodnr;	/* DMA 4 LIODN */
	u8	res23[48];
	u8	res24[64];
	u32	pblsr;		/* Preboot loader status */
	u32	pamubypenr;	/* PAMU bypass enable */
	u32	dmacr1;		/* DMA control */
	u8	res25[4];
	u32	gensr1;		/* General status */
	u8	res26[12];
	u32	gencr1;		/* General control */
	u8	res27[12];
	u8	res28[4];
	u32	cgensrl;	/* Core general status */
	u8	res29[8];
	u8	res30[4];
	u32	cgencrl;	/* Core general control */
	u8	res31[184];
	u32	sriopstecr;	/* SRIO prescaler timer enable control */
	u32	dcsrcr;		/* DCSR Control register */
	u8	res31a[56];
	u32	tp_ityp[64];	/* Topology Initiator Type Register */
	struct {
		u32	upper;
		u32	lower;
	} tp_cluster[16];	/* Core Cluster n Topology Register */
	u8	res32[1344];
	u32	pmuxcr;		/* Pin multiplexing control */
	u8	res33[60];
	u32	iovselsr;	/* I/O voltage selection status */
	u8	res34[28];
	u32	ddrclkdr;	/* DDR clock disable */
	u8	res35;
	u32	elbcclkdr;	/* eLBC clock disable */
	u8	res36[20];
	u32	sdhcpcr;	/* eSDHC polarity configuration */
	u8	res37[380];
} ccsr_gur_t;

#endif
