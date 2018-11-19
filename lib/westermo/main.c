#define pr_fmt(fmt) "libwestermo: " fmt

#include <common.h>
#include <globalvar.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <net.h>
#include <of.h>

#include <generated/utsrelease.h>

static char idmem[128];

#if defined(CONFIG_CORAZON) || defined(CONFIG_CORONET) || defined(CONFIG_MACH_DAGGER)

#define PRODUCT_FMT "%.4x"

static u32 wmo_product(void)
{
	return (idmem[116] << 8) | idmem[117];
}

#elif defined(CONFIG_MACH_BASIS)

#define PRODUCT_FMT "%.6x"

static u32 wmo_product(void)
{
	return (idmem[2] << 16 | idmem[3] << 8 | idmem[5]);
}

#else

#error "Not a westermo product!"

#endif


static void set_product(int product)
{
	char str[10];
	snprintf (str, sizeof(str), PRODUCT_FMT, product);

	globalvar_add_simple("wmo.product", str);
}

static const char *wmo_mac(void)
{
	return &idmem[34];
}

static int wmo_of_fixup_mac(struct device_node *np)
{
	u8 mac[6];
	u32 idx;

	if (of_property_read_u8_array(np, "local-mac-address", mac, 6))
		return -EINVAL;

	if (is_valid_ether_addr(mac))
		return 0;

	if (of_property_read_u32_index(np, "cell-index", 0, &idx)) {
		if (of_property_read_u32_index(np, "linux,network-index", 0, &idx))
			return -EINVAL;
	}

	memcpy(mac, wmo_mac(), 6);
	ethaddr_add(&mac, idx);

	return of_property_write_u8_array(np, "local-mac-address", mac, 6);
}

static int wmo_of_fixup(struct device_node *root, void *arg)
{
	struct device_node *np;
	int err;
	const char *label;

	/* Communicate the active partition to WeOS by reading the
	 * global variable setup by the wmoboot script */
	label = globalvar_get_match("wmo.boot_partition", "");
	if (label) {
		pr_info("%s - boot_partition:%s\n", __FUNCTION__, label);
		
		np = of_find_node_by_name(root, "u-boot");
		if (!np)
			np = of_find_node_by_name(root, "chosen");

		if (np) {

			of_property_write_u8_array(np, "boot_partition", label,
						   strlen(label) + 1);
			of_property_write_u8_array(np, "version", UTS_RELEASE,
						   strlen(UTS_RELEASE) + 1);
		}
	}

	if (of_device_is_compatible(root, "wmo,Corazon"))
		goto out;

	np = of_find_node_by_type(root, "network");
	while (np) {
		if (!of_device_is_available(np))
			goto next;

		err = wmo_of_fixup_mac(np);
		if (err) {
			pr_warn("%s: unable to assign MAC address err %d\n",
				np->full_name, err);
		}
	next:
		np = of_find_node_by_type(np, "network");
	}

out:
	printf("\e[;1m[ OK ]\e[0m\n\n");
	return 0;
}

static int wmo_init(void)
{
	char macstr[sizeof("xx:xx:xx:xx:xx:xx")];
	struct eth_device *edev;
	int fd;
	static u32 product;

	fd = open(CONFIG_WMO_IDMEM, O_RDONLY);
	if ((fd < 0) || (read(fd, idmem, sizeof(idmem)) != sizeof(idmem))) {
		pr_err("unable to read idmem\n");
		product = 0;
	} else {
		product = wmo_product();
	}

	set_product(product);

	ethaddr_to_string(wmo_mac(), macstr);
	edev = eth_get_current();
	if (edev)
		dev_set_param(&edev->dev, "ethaddr", macstr);
	else
		pr_warn("warning, no active interface, mac not set\n");

	of_register_fixup(wmo_of_fixup, NULL);
	pr_info("product:%.6x mac:%s\n", wmo_product(), macstr);
	return 0;
}
late_initcall(wmo_init);

static int wmo_banner(void)
{
	char *release = basprintf("Barebox %s", UTS_RELEASE);

	printf("\n\n%s \e[;1m%.*s\e[0m\n", release, 66 - strlen(release),
	       "========================================================");
	free(release);
	return 0;
}
postconsole_initcall(wmo_banner);
