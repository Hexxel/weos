#define pr_fmt(fmt) "libwestermo: " fmt

#include <common.h>
#include <mv88e6xxx.h>

#include <linux/phy.h>

static struct phy_device *wmo_sc_register(struct mii_bus *smi, int addr,
					  void *priv)
{
	struct phy_device *sc;
	int err;

	sc = phy_device_create(smi, addr, 0x88e6fff0);
	if (!sc) {
		pr_err("unable to create sc device (address:%d)\n", addr);
		return NULL;
	}

	sc->priv = priv;
	smi->phy_map[addr] = sc;

	err = phy_register_device_nogen(sc);
	if (err)
		return NULL;

	return sc;
}

int wmo_backplane_setup(struct mv88e6xxx_pdata *backplane)
{
	struct device_d *smidev;
	struct mii_bus *smi;
	struct phy_device *sc0;
	int i;

	smidev = get_device_by_name("miibus0");
	if (!smidev) {
		pr_err("unable to locate the SMI bus\n");
		return -ENODEV;
	}

	smi = to_mii_bus(smidev);

	for (i = 1; backplane[i].cpu_speed; i++) {
		backplane[0].slaves[i - 1] =
			wmo_sc_register(smi, backplane[i].smi_addr,
					&backplane[i]);

		if (!backplane[0].slaves[i - 1]) {
			pr_info("backplane: %d switchcores probed\n", i - 1);
			break;
		}
	}

	sc0 = wmo_sc_register(smi, backplane[0].smi_addr, &backplane[0]);
	if (!sc0)
		return -ENODEV;

	return 0;
}
