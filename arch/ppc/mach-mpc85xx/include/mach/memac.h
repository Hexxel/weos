#ifndef __MACH_MEMACH_H
#define __MACH_MEMACH_H

struct memac_mdio_pdata {
	uint mdc_freq;		/* desired output clock freq, default 2.5 MHz */
	uint enet_freq;		/* input clock freq to ethernet block */
};

#endif	/* __MACH_MEMACH_H */
