#ifndef __MDIO_SMI_H
#define __MDIO_SMI_H

#define SMI_G1 0x1b
#define SMI_G1_GS 0
#define SMI_G1_GS_PPUEN   (1 << 14)
#define SMI_G1_GS_PPUDONE (1 << 15)
#define SMI_G1_GS_PPUMODE (SMI_G1_GS_PPUEN | SMI_G1_GS_PPUDONE)
#define SMI_G1_GC 4
#define SMI_G1_GC_PPU     (1 << 14)

#define SMI_G2 0x1c
#define SMI_G2_PC 0x18
#define SMI_G2_PC_B (1 << 15)
#define SMI_G2_PD 0x19

struct smi_bus_pdata {
	u8              addr;
	struct mii_bus *bus;
};

#endif	/* __MDIO_SMI_H */
