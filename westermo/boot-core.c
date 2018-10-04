#include <generated/utsrelease.h>
#include "boot-version.h"
#include "drg-spi.h"

#define UART_BASE 0xd0012000
#define UART_THR  0
#define UART_LSR  0x14
#define UART_LSR_THRE (1 << 5)

#define DAGGER_PBL_JUMP (1 << 0)

int puts(const char *msg)
{
	volatile u32 *thr = (void *)(UART_BASE + UART_THR);
	volatile u32 *lsr = (void *)(UART_BASE + UART_LSR);

	for (; *msg; msg++) {
		*thr = *msg;
		while (!(*lsr & UART_LSR_THRE));
	}

	return 0;
}

void memcpy32(u32 *dst, const u32 *src, u32 n)
{
	for (; n; n -= 4)
		*(dst++) = *(src++);
}

/* copies the dragonite's spi firmware to its instruction ram, done in
 * assembler due to the author's lack of .got section fixup skills */
void _drg_spi_copy(u32 itcm_base);

int dragonite_init(void)
{
	drg_mem_t *mem = (void *)0xb4000000;
	volatile u32 *poe_ctrl = (void *)0xb800001c;
	volatile u32 *gpio_out = (void *)0xd0018100;
	volatile u32 *gpio_out_en = (void *)0xd0018104;
	volatile u32 data;

	/* disable dragonite core */
	*poe_ctrl &= ~(1 << 1);

	_drg_spi_copy(0xb0000000);

	/* enable dragonite core */
	*poe_ctrl |= (1 << 1);

	/* wait for dragonite fw to ack that it is running */
	while (!(mem->flags & DRG_SPI_INIT_OK));

	if (DRG_SPI_VERSION_MAJOR(mem->version) !=
	    DRG_SPI_VERSION_MAJOR(DRG_SPI_VERSION))
		return -1;

	/* 20 MHz clock */
	mem->conf = 0x15;

	/* Set the CS to output for SPI-flash memory DRG. */
	data = *gpio_out_en;
	data &= ~DRG_SPI_GPIO_CS;
	*gpio_out_en = data;

	/* Set the CS to low for SPI-flash memory DRG. */
	data = *gpio_out;
	data &= ~DRG_SPI_GPIO_CS;
	*gpio_out = data;
	return 0;
}

void bootloader_copy(void *dst, void *src, u32 len)
{
	drg_mem_t *mem = (void *)0xb4000000;
	u32 offs = (u32)src;

	mem->txdata[0] = ((offs & 0xff) << 8) | 0x0c;
	mem->txdata[1] = (offs >>  8) & 0xffff;
	mem->txdata[2] = (offs >> 24);
	mem->len = 6;

	mem->flags = DRG_SPI_OWNER_DRG | DRG_SPI_FIRST;

	while(mem->flags & DRG_SPI_OWNER_DRG);

	mem->txdata[0] = 0;
	mem->txdata[1] = 0;
	mem->txdata[2] = 0;
	mem->len = sizeof(mem->rxdata);

	offs = (u32)dst;

	/* for (offs = 0; offs < 0xbf000; offs += sizeof(mem->rxdata)) { */
	for (; len > sizeof(mem->rxdata); len -= sizeof(mem->rxdata)) {
		mem->flags = DRG_SPI_OWNER_DRG;
		while(mem->flags & DRG_SPI_OWNER_DRG);
		memcpy32((u32 *)offs, (u32 *)mem->rxdata, sizeof(mem->rxdata));
		offs += sizeof(mem->rxdata);
	}

	mem->flags = DRG_SPI_OWNER_DRG | DRG_SPI_LAST;
	while(mem->flags & DRG_SPI_OWNER_DRG);
	memcpy32((u32 *)offs, (u32 *)mem->rxdata, sizeof(mem->rxdata));
}

extern int strlen(const char *s);
static char *bb_banner = "\r\n\e[1mDagger PBL " UTS_RELEASE "\e[0m ";

int pbl_main(int argc, char **_argv)
{
	/* argv is in fact a list of u32's, but gcc complains if the
	 * prototype for main() differs from the standard. */
	u32 *argv = (void *)_argv;

	void (*dst)(void);
	void *src;
	u32 len, flags;
	int err;
	int i, sz;

	sz = strlen(bb_banner);
	puts(bb_banner);

	for (i = 0; i < (77 - sz); i++)
		puts("=");
	puts("\r\n");

	puts("Validating arguments ....................................... ");

	if (argc < 4) {
		puts("\e[7m[FAIL]\e[0m\r\n");
		goto err;
	} else {
		dst   = (void *)argv[0];
		src   = (void *)argv[1];
		len   = argv[2];
		flags = argv[3];
		puts("\e[1m[ OK ]\e[0m\r\n");
	}

	puts("Initializing SPI co-processor .............................. ");
	err = dragonite_init();
	if (err) {
		puts("\e[7m[FAIL]\e[0m\r\n");
		goto err;
	} else {
		puts("\e[1m[ OK ]\e[0m\r\n");
	}

	puts("Loading bootloader ......................................... ");
	bootloader_copy(dst, src, len);
	puts("\e[1m[ OK ]\e[0m\r\n");

	if (flags & DAGGER_PBL_JUMP)
		dst();
        
	return 0;
err:
	while(1);
}
