/*
 * drivers/watchdog/dagger_wdt.c
 *
 * Westermo Dagger watchdog driver
 *
 * Copyright (C) 2017 Westermo Teleindustri AB
 *
 * Author: Joacim Zetterling <joacim.zetterling@westermo.se>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>

#include <poller.h>
#include <gpio.h>
#include <of.h>
#include <of_gpio.h>

#define WDT_DEFAULT_INTERVAL  (100 * MSECOND)

struct dagger_wdt {
	struct device_d *pdev;
	struct watchdog wd;
	struct poller_struct poll;
	u64 timeout;
	u32 pin;
	bool next;
	bool initialized;
};

static void dagger_wdt_kick(struct poller_struct *poll)
{
	struct dagger_wdt *priv = container_of(poll, struct dagger_wdt, poll);
	u64 now = get_time_ns();

	if ((priv->timeout == 0) || (now < priv->timeout) ||
		((now + WDT_DEFAULT_INTERVAL) < now))
		return;

	gpio_set_value(priv->pin, priv->next);
	priv->next = !priv->next;
	priv->timeout = get_time_ns() + WDT_DEFAULT_INTERVAL;
	if (!priv->timeout) /* too avoid disable state */
		priv->timeout = get_time_ns() + WDT_DEFAULT_INTERVAL;
}

static int dagger_wdt_init(struct watchdog *wd, unsigned timeout)
{
	struct dagger_wdt *priv = container_of(wd, struct dagger_wdt, wd);
	struct device_node *np = priv->pdev->device_node;
	u64 now = get_time_ns();

	/* Setup watchdog GPIO.
		Due to initialization order of the DTS entries,
		the GPIO probe/init is done after the watchdog probe.
		Solution is to move the GPIO watchdog init here. (JZE) */
	if (!priv->initialized) {
		priv->pin = of_get_named_gpio(np, "gpios", 0);
		if (priv->pin < 0) {
			dev_err(priv->pdev, "error: missing gpio\n");
			return -EINVAL;
		}
		priv->initialized = true;
	}
	gpio_direction_output(priv->pin, 1);
	gpio_set_value(priv->pin, priv->next);
	priv->next = !priv->next;

	if (timeout || (priv->timeout == -1))
		priv->timeout = now + WDT_DEFAULT_INTERVAL;
	else
		priv->timeout = 0;

	return 0;
}

static int dagger_wdt_probe(struct device_d *pdev)
{
	struct dagger_wdt *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));
	if (!priv) {
		dev_err(pdev, "error: xzalloc: %d\n", -ENOMEM);
		return -ENOMEM;
	}

	priv->pdev = pdev;
	priv->wd.set_timeout = dagger_wdt_init;
	priv->poll.func = dagger_wdt_kick;
	priv->timeout = -1;
	priv->next = false;
	priv->initialized = false;

	ret = poller_register(&priv->poll);
	if (ret) {
		dev_err(priv->pdev, "cannot register poller (errno)\n");
		goto err_free;
	}

	ret = watchdog_register(&priv->wd);
	if (ret < 0) {
		dev_err(pdev, "cannot register watchdog device\n");
		goto err_free;
	}

	dev_info(pdev, "probed\n");
	return ret;

err_free:
	free(priv);
	return ret;
}


static __maybe_unused struct of_device_id dagger_wdt_of_match[] = {
	{ .compatible = "wmo,dagger-wdt", },
	{},
};

static struct driver_d platform_wdt_driver = {
	.name = "dagger-wdt",
	.of_compatible = DRV_OF_COMPAT(dagger_wdt_of_match),
	.probe = dagger_wdt_probe,
};
device_platform_driver(platform_wdt_driver);

MODULE_AUTHOR("Joacim Zetterling");
MODULE_DESCRIPTION("Westermo Dagger Watchdog Driver");
MODULE_LICENSE("GPL");
