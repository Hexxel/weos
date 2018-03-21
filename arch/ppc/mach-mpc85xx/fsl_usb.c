#include <common.h>

#include <asm/io.h>

#include <mach/fsl_usb.h>
#include <mach/immap_85xx.h>

#define PLLPRG2_PLLEN		(1 << 21)
#define PLLPRG2_INPUT_CLK_SEL	(1 << 20)
#define PLLPRG2_MP(_m)		((_m) << 14)
#define PLLPRG2_CTRL2       (1 << 13)
#define PLLPRG2_REF_DIV(_d)	((_d) << 4)
#define PLLPRG2_PHY1_CLK_EN	(1 << 1)
#define PLLPRG2_PHY2_CLK_EN	(1 << 0)

#define USB_PHY_CTRL_OFFSET		0
#define USB_PHY_CTRL_USBPEN		(1 << 0)
#define USB_PHY_DRVVBUS_OFFSET		4
#define USB_PHY_DRVVBUS_ACTIVE_LOW	(1 << 0)
#define USB_PHY_DRVVBUS_EN		(1 << 1)
#define USB_PHY_PWRFLT_OFFSET		8
#define USB_PHY_PWRFLT_ACTIVE_LOW	(1 << 0)
#define USB_PHY_PWRFLT_EN		(1 << 1)

static void fsl_usb_phy_port_enable(struct fsl_usb_phy_port_pdata *ppi,
					void __iomem *base)
{
	u32 drvvbus = USB_PHY_DRVVBUS_EN;
	u32 pwrflt  = USB_PHY_PWRFLT_EN;

	if (!ppi->enable)
		return;

	if (ppi->drvvbus_act_low)
		drvvbus |= USB_PHY_DRVVBUS_ACTIVE_LOW;

	if (ppi->pwrflt_act_low)
		pwrflt |= USB_PHY_PWRFLT_ACTIVE_LOW;

	setbits_be32(base + USB_PHY_CTRL_OFFSET, USB_PHY_CTRL_USBPEN);
	setbits_be32(base + USB_PHY_DRVVBUS_OFFSET, drvvbus);
	setbits_be32(base + USB_PHY_PWRFLT_OFFSET, pwrflt);
}

void fsl_usb_phy_init(struct fsl_usb_phy_pdata *pi)
{

	void __iomem *regs = (void __iomem *)MPC85xx_USB_PHY_ADDR;
	u32 prg2;

	prg2 = PLLPRG2_PLLEN |
		(pi->port1.enable ? PLLPRG2_PHY1_CLK_EN : 0) |
		(pi->port2.enable ? PLLPRG2_PHY2_CLK_EN : 0);

	if (pi->internal_clk)
		prg2 |= PLLPRG2_INPUT_CLK_SEL |
			PLLPRG2_MP(24) | PLLPRG2_REF_DIV(5);
	else
		prg2 |= PLLPRG2_MP(20) | PLLPRG2_REF_DIV(1);

#ifdef CONFIG_CORONET
	/* Clock fix from FSL */
	prg2 = PLLPRG2_INPUT_CLK_SEL | PLLPRG2_CTRL2 | PLLPRG2_MP(24) |
		PLLPRG2_REF_DIV(5) | PLLPRG2_PHY1_CLK_EN;
	out_be32(regs + MPC85xx_USB_PHY_PLLPRG1_OFFSET, 0x00000001);
	out_be32(regs + MPC85xx_USB_PHY_PLLPRG2_OFFSET, prg2);
	prg2 |= PLLPRG2_PLLEN;
#endif

	out_be32(regs + MPC85xx_USB_PHY_PLLPRG2_OFFSET, prg2);

	fsl_usb_phy_port_enable(&pi->port1, (void __iomem *)MPC85xx_USB_PHY_P1_ADDR);
	fsl_usb_phy_port_enable(&pi->port2, (void __iomem *)MPC85xx_USB_PHY_P2_ADDR);
}
