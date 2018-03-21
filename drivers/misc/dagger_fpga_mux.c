/*
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <clock.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>


struct dagger_fpga_mux_data {
	u32 mux_reg;
	u32 mux_val;
	u32 mux_old;
	u8  *writebuf;
	struct i2c_client *client;
};

#define DAGGER_FPGA_MUX_OFFS_SYS_FW  0x00
#define DAGGER_FPGA_MUX_OFFS_IO_FW   0x04
#define DAGGER_FPGA_MUX_OFFS_RST_FW  0x08
#define DAGGER_FPGA_MUX_OFFS_MUX     0x0c

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
static unsigned io_limit = 128;

/*
 * Specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
//static unsigned write_timeout = 25;


static ssize_t dagger_fpga_mux_read(struct dagger_fpga_mux_data *dc, char *buf,
		unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	struct i2c_client *client;
	int status;

	memset(msg, 0, sizeof(msg));

	client = dc->client;

	msgbuf[0] = offset;

	msg[0].addr = client->addr;
	msg[0].buf = msgbuf;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = count;

	mdelay(1);

	status = i2c_transfer(client->adapter, msg, 2);
	if (status == 2) {
		
		return count;
	}

	return -ETIMEDOUT;
}

static ssize_t dagger_fpga_mux_write(struct dagger_fpga_mux_data *dc, const char *buf,
		unsigned offset, size_t count)
{
	struct i2c_client *client;
	struct i2c_msg msg;
	ssize_t status;

	client = dc->client;

	msg.addr = client->addr;
	msg.flags = 0;

	/* msg.buf is u8 and casts will mask the values */
	msg.buf = dc->writebuf;
	msg.buf[0] = offset;
	memcpy(&msg.buf[1], buf, count);
	msg.len = count + 1;

	mdelay(1);

	status = i2c_transfer(client->adapter, &msg, 1);
	if (status == 1)
		return count;

	return -ETIMEDOUT;
}

static int dagger_fpga_mux_probe(struct device_d *pdev)
{
	struct i2c_client *client = to_i2c_client(pdev);
	struct dagger_fpga_mux_data *dc;
	u32 tmp;
	char buf[4];

	dc = xzalloc(sizeof(struct dagger_fpga_mux_data));
	if (!dc) {
		dev_err(pdev, "error: xzalloc: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	pdev->priv = dc;
	dc->client = client;
	dc->mux_reg = -1;
	dc->mux_val = -1;
	dc->mux_old = -1;
	
	/* buffer (data + address at the beginning) */
	dc->writebuf = xmalloc(io_limit + 2);

	if (!of_property_read_u32(pdev->device_node, "mux-reg", &tmp))
		dc->mux_reg = tmp;
	if (!of_property_read_u32(pdev->device_node, "mux-val", &tmp))
		dc->mux_val = tmp;

	if (dc->mux_reg != -1) {
		/* Show FPGA firmware version */
		tmp = (int)dagger_fpga_mux_read(dc, buf, DAGGER_FPGA_MUX_OFFS_SYS_FW, 4);
		if (tmp > 0)
			dev_info(pdev, "%8.8x\n", *(int *)buf);

		/* Set FPGA mux to enable config flash (0x0d@0x0c), see FPGA spec. ??? */
		tmp = dagger_fpga_mux_read(dc, buf, dc->mux_reg, 1);
		if (tmp > 0) {
			dc->mux_old = buf[0];
			dagger_fpga_mux_write(dc, (const char *)&dc->mux_val, dc->mux_reg, 1);
		}
	}

	dev_info(pdev, "probed\n");
	return 0;
}

static void dagger_fpga_mux_remove(struct device_d *pdev)
{
	struct dagger_fpga_mux_data *dc = pdev->priv;

	/* Restore FPGA mux (0x0c@0x0c), see FPGA spec. ??? */
	if (dc->mux_old != 1)
		dagger_fpga_mux_write(dc, (const char *)&dc->mux_old, dc->mux_reg, 1);

	kfree(dc);
	//dev_info(pdev, "removed\n");
}

static __maybe_unused struct of_device_id dagger_fpga_mux_of_match[] = {
	{ .compatible = "wmo,dagger-fpga-mux", },
	{},
};

static struct driver_d dagger_fpga_mux_driver = {
	.name		= "dagger-fpga-mux",
	.of_compatible = DRV_OF_COMPAT(dagger_fpga_mux_of_match),
	.probe		= dagger_fpga_mux_probe,
	.remove		= dagger_fpga_mux_remove,
};

static int dagger_fpga_mux_init(void)
{
	i2c_driver_register(&dagger_fpga_mux_driver);
	return 0;
}
device_initcall(dagger_fpga_mux_init);
