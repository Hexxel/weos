#ifndef _DRG_SPI_H
#define _DRG_SPI_H

#define ITCM_BASE 0
#define ITCM_SIZE (64 << 10)

#define DTCM_BASE 0x04000000
#define DTCM_SIZE (32 << 10)

#define DRG_IO_BASE  0x08080000

#define DRG_GPP_CTRL (DRG_IO_BASE +  0x10)
#define DRG_GPP_CTRL_DOORBELL (1 << 8)

#define DRG_SPI_BASE (DRG_IO_BASE + 0x600)

#define DRG_SPI_GPIO_CS (1 << 24)

#ifndef __ASSEMBLER__

#define DRG_SPI_VERSION_NO(_M, _m, _p) (((_M) << 16) | ((_m) << 8) | (_p))
#define DRG_SPI_VERSION_MAJOR(_ver)    ((_ver) >> 16)

#define DRG_SPI_BLOCK_SZ 0x1000

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

typedef volatile struct drg_spi {
	u32 ctrl;
#define DRG_SPI_CTRL_CS    (1 << 0)
#define DRG_SPI_CTRL_READY (1 << 1)
	u32 conf;
#define DRG_SPI_CONF_16BIT (1 << 5)
	u32 tx;
	u32 rx;
	u32 irq_cause;
	u32 irq_mask;
} drg_spi_t;

/* This struct defines the register interface between the Dragonite
 * and the CPU. The `version` field is required to be at offset 0. If
 * any changes are made, be sure to bump the version. If the changes
 * are not compatible with the old interface, bump the major
 * version. */
typedef volatile struct drg_mem {
	u32 version;
#define DRG_SPI_VERSION DRG_SPI_VERSION_NO(1, 0, 0)

	u32 flags;
#define DRG_SPI_INIT_OK   (1 << 31)
#define DRG_SPI_IRQ       (1 << 4)
#define DRG_SPI_ERROR     (1 << 3)
#define DRG_SPI_LAST      (1 << 2)
#define DRG_SPI_FIRST     (1 << 1)
#define DRG_SPI_OWNER_DRG (1 << 0)

	u32 conf;
	u32 len;

	/* reserve space for more control registers */
	u32 reserved[60];

	/* offset 0x100 */
	u16 txdata[DRG_SPI_BLOCK_SZ >> 1];
	u16 rxdata[DRG_SPI_BLOCK_SZ >> 1];
} __attribute__ ((packed)) drg_mem_t;

#endif	/* __ASSEMBLER__ */

#endif	/* _DRG_SPI_H */
