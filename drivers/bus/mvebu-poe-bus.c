#include <driver.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_address.h>

static int mvebu_poe_probe(struct device_d *pdev)
{
	struct device_node *np = pdev->device_node;
	u32 __iomem *poe_ctrl;
	int err;

	poe_ctrl = of_iomap(np, 0);
	if (!poe_ctrl) {
		pr_err("error: of_iomap: %d\n", -EINVAL);
		return -EINVAL;
	}

	/* enable PoE subsystem and AHB loop */
	writel(0x384a086d, poe_ctrl);

	err = of_platform_populate(np, NULL, pdev);
	if (err < 0)
		return err;

	pr_info("%s: ok\n", dev_name(pdev));
	return 0;
}

static struct of_device_id mvebu_poe_dt_ids[] = {
	{ .compatible = "marvell,poe-bus" },
	{ }
};

static struct driver_d mvebu_poe_driver = {
	.name   = "mvebu_poe",
	.probe  = mvebu_poe_probe,
	.of_compatible = DRV_OF_COMPAT(mvebu_poe_dt_ids),
};

device_platform_driver(mvebu_poe_driver);
