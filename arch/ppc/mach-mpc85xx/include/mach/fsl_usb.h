#ifndef _MACH_FSL_USB_H
#define _MACH_FSL_USB_H

struct fsl_usb_phy_port_pdata {
	int enable:1;
	int drvvbus_act_low:1;
	int pwrflt_act_low:1;
};

struct fsl_usb_phy_pdata {
	int internal_clk:1;

	struct fsl_usb_phy_port_pdata port1;
	struct fsl_usb_phy_port_pdata port2;
};

void fsl_usb_phy_init(struct fsl_usb_phy_pdata *pi);

#endif	/* _MACH_FSL_USB_H */
