#include "drg-spi.h"

static inline void doorbell(int on)
{
	volatile u32 *gpp_ctrl = (void *)DRG_GPP_CTRL;

	if (on)
		*gpp_ctrl |=  DRG_GPP_CTRL_DOORBELL;
	else
		*gpp_ctrl &= ~DRG_GPP_CTRL_DOORBELL;
}

static inline void spi_transfer_odd(drg_spi_t *spi,
				    volatile u16 *txdata, volatile u16 *rxdata)
{
	spi->conf &= ~DRG_SPI_CONF_16BIT;

	spi->tx = *txdata;
	while (!(spi->ctrl & DRG_SPI_CTRL_READY));

	*rxdata = spi->rx;
}

void spi_transfer(drg_spi_t *spi, drg_mem_t *mem)
{
	volatile u16 *rxdata = mem->rxdata;
	volatile u16 *txdata = mem->txdata;
	int i, wlen = mem->len & ~1;

	spi->conf = mem->conf;

	if (mem->flags & DRG_SPI_FIRST)
		spi->ctrl |= DRG_SPI_CTRL_CS;
	
	if (mem->len == 1) {
		spi_transfer_odd(spi, txdata, rxdata);
		goto done;
	}

	spi->conf |= DRG_SPI_CONF_16BIT;
	for (i = 0; i < wlen; i += 2) {
		spi->tx = *(txdata++);
		while (!(spi->ctrl & DRG_SPI_CTRL_READY));
		*(rxdata++) = spi->rx;
	}

	if (mem->len & 1)
		spi_transfer_odd(spi, txdata, rxdata);

done:
	if (mem->flags & DRG_SPI_LAST)
		spi->ctrl &= ~DRG_SPI_CTRL_CS;
}

int main(void)
{
	drg_mem_t *mem = (void *)DTCM_BASE;
	drg_spi_t *spi = (void *)DRG_SPI_BASE;

	mem->version = DRG_SPI_VERSION;
	mem->flags |= DRG_SPI_INIT_OK;

	while (1) {
		while (!(mem->flags & DRG_SPI_OWNER_DRG));

		if (mem->len > sizeof(mem->txdata)) {
			mem->flags |= DRG_SPI_ERROR;
			mem->flags &= ~DRG_SPI_OWNER_DRG;
			continue;
		}
			
		spi_transfer(spi, mem);

		mem->flags &= ~DRG_SPI_OWNER_DRG;

		if (mem->flags & DRG_SPI_IRQ) {
			doorbell(1);
			while (mem->flags & DRG_SPI_IRQ);
			doorbell(0);
		}
	}

	return 0;
}
