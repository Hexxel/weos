/*
 * Westermo Dragonite SPI controller
 *
 * Tobias Waldekranz <tobias@waldekranz.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <malloc.h>
#include <spi/spi.h>
#include <gpio.h>
#include <of_gpio.h>

#define DRG_SPI_VERSION 1

typedef struct drg_mem {
	u32 version;
#define DRG_SPI_VERSION_MAJOR(_ver) ((_ver) >> 16)

	u32 flags;
#define DRG_SPI_INIT_OK   (1 << 31)
#define DRG_SPI_ERROR     (1 << 3)
#define DRG_SPI_LAST      (1 << 2)
#define DRG_SPI_FIRST     (1 << 1)
#define DRG_SPI_OWNER_DRG (1 << 0)

	u32 conf;
	u32 len;

	u32 reserved[60];

	/* offset 0x100 */
#define DRG_SPI_BLOCK_SZ 0x1000
	u16 txdata[DRG_SPI_BLOCK_SZ >> 1];
	u16 rxdata[DRG_SPI_BLOCK_SZ >> 1];
} __packed drg_mem_t;


struct drg_spi {
	struct spi_master master;
	u32 __iomem *poe_ctrl;
	drg_mem_t __iomem *mem;
	struct clk *clk;
	u32 cs;
};

#define dspi_from_spi(_spi) container_of((_spi)->master, struct drg_spi, master)

static int drg_spi_set_speed(struct drg_spi *dspi, u32 speed)
{
	u32 pscl, conf;

	/* standard prescaler values: 1,2,4,6,...,30 */
	pscl = DIV_ROUND_UP(clk_get_rate(dspi->clk), speed);
	pscl = roundup(pscl, 2);

	if (pscl > 30)
		return -EINVAL;

	conf  = readl(&dspi->mem->conf) & ~0x1f;
	conf |= BIT(4) | (pscl >> 1);
	writel(conf, &dspi->mem->conf);
	return 0;
}


static int drg_spi_command(struct drg_spi *dspi, u32 flags,
			   struct spi_transfer *t)
{
	int err;

	if (t && t->speed_hz) {
		err = drg_spi_set_speed(dspi, t->speed_hz);
		if (err)
			return err;
	}

	if (t) {
		writel(t->len, &dspi->mem->len);
		memcpy(&dspi->mem->txdata, t->tx_buf, t->len);
	} else {
		writel(0, &dspi->mem->len);
	}

	writel(DRG_SPI_OWNER_DRG | flags, &dspi->mem->flags);

	while (readl(&dspi->mem->flags) & DRG_SPI_OWNER_DRG);

	if (readl(&dspi->mem->flags) & DRG_SPI_ERROR)
		return -EIO;

	if (t && t->rx_buf)
		memcpy(t->rx_buf, &dspi->mem->rxdata, t->len);

	return 0;
}

static int drg_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct drg_spi *dspi = dspi_from_spi(spi);
	struct spi_transfer *t;
	int err;

	gpio_set_value(dspi->cs, 0);
	err = drg_spi_command(dspi, DRG_SPI_FIRST, NULL);
	if (err)
		return err;

	msg->actual_length = 0;

	list_for_each_entry(t, &msg->transfers, transfer_list) {
		err = drg_spi_command(dspi, 0, t);
		if (err)
			break;
		msg->actual_length += t->len;
	}

	err = drg_spi_command(dspi, DRG_SPI_LAST, NULL);
	gpio_set_value(dspi->cs, 1);
	return err;
}

static int drg_spi_setup(struct spi_device *spi)
{
	struct drg_spi *dspi = dspi_from_spi(spi);
	int err;

	err = drg_spi_command(dspi, DRG_SPI_LAST, NULL);
	if (err)
		return err;

	if ((spi->mode & (SPI_CPOL|SPI_CPHA)) != SPI_MODE_0)
		return -ENOSYS;

	return drg_spi_set_speed(dspi, spi->max_speed_hz);
}

static struct of_device_id drg_spi_match_table[] = {
	{ .compatible = "wmo,drg-spi" },

	{}
};

static int drg_spi_probe(struct device_d *dev)
{
	struct drg_spi *dspi;
	struct spi_master *master;
	int err = 0;

	dspi = xzalloc(sizeof(*dspi));

	dspi->mem = dev_request_mem_region(dev, 0);
	if (IS_ERR(dspi->mem)) {
		err = PTR_ERR(dspi->mem);
		dev_err(dev, "error: missing dragonite registers\n");
		goto err_free;
	}

	dspi->poe_ctrl = dev_request_mem_region(dev, 1);
	if (IS_ERR(dspi->poe_ctrl)) {
		err = PTR_ERR(dspi->poe_ctrl);
		dev_err(dev, "error: missing poe ctrl register\n");
		goto err_free;
	}

	/* Enables the PoE subsystem, PBL should have taken care of
	 * this, but in case it hasn't (e.g. during development) the
	 * system will HARD lock on any register access. */
	writel(readl(dspi->poe_ctrl) | BIT(0), dspi->poe_ctrl);
	mdelay(1);

	if (DRG_SPI_VERSION_MAJOR(readl(&dspi->mem->version)) !=
	    DRG_SPI_VERSION) {
		err = -EINVAL;
		dev_err(dev, "error: wrong dragonite firmware version %#x, "
			"expected major version %d\n",
			readl(&dspi->mem->version), DRG_SPI_VERSION);

		goto err_free;
	}

	dspi->clk = clk_get(dev, NULL);
	if (IS_ERR(dspi->clk)) {
		err = PTR_ERR(dspi->clk);
		dev_err(dev, "error: missing clock\n");
		goto err_free;
	}

	dspi->cs = of_get_named_gpio(dev->device_node, "gpios", 0);
	if (dspi->cs < 0) {
		err = dspi->cs ? : -EINVAL;
		dev_err(dev, "error: missing spi cs\n");
		goto err_free;
	}

	gpio_direction_output( dspi->cs, 1);
	gpio_set_value(dspi->cs, 1);

	master = &dspi->master;
	master->dev = dev;
	master->bus_num = dev->id;
	master->setup = drg_spi_setup;
	master->transfer = drg_spi_transfer;
	master->num_chipselect = 1;

	return spi_register_master(master);

err_free:
	free(dspi);
	return err;
}

static struct driver_d drg_spi_driver = {
	.name  = "drg-spi",
	.probe = drg_spi_probe,
	.of_compatible = DRV_OF_COMPAT(drg_spi_match_table),
};
device_platform_driver(drg_spi_driver);
