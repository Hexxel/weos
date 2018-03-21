#define DEBUG

#include <common.h>
#include <command.h>
#include <image.h>
#include <init.h>
#include <malloc.h>
#include <memory.h>
#include <environment.h>
#include <asm/bitops.h>
#include <asm/processor.h>
#include <boot.h>
#include <bootm.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <fit.h>
#include <linux/sizes.h>
#include <linux/types.h>
#include <driver.h>
#include <restart.h>

static int bootm_relocate_fdt(void *addr, struct image_data *data)
{
	if (addr < LINUX_TLB1_MAX_ADDR) {
		/* The kernel is within  the boot TLB mapping.
		 * Put the DTB above if there is no space
		 * below.
		 */
		if (addr < (void *)data->oftree->totalsize) {
			addr = (void *)PAGE_ALIGN((phys_addr_t)addr +
					data->os->header.ih_size);
			addr += data->oftree->totalsize;
			if (addr < LINUX_TLB1_MAX_ADDR)
				addr = LINUX_TLB1_MAX_ADDR;
		}
	}

	if (addr > LINUX_TLB1_MAX_ADDR) {
		pr_crit("Unable to relocate DTB to Linux TLB\n");
		return 1;
	}

	addr = (void *)PAGE_ALIGN_DOWN((phys_addr_t)addr -
			data->oftree->totalsize);
	memcpy(addr, data->oftree, data->oftree->totalsize);
	free(data->oftree);
	data->oftree = addr;

	pr_info("Relocating device tree to 0x%p\n", addr);
	return 0;
}

static int do_bootm_initrd(struct image_data *data)
{
	int fd;
	void *mem;

	if (data->initrd_address != UIMAGE_INVALID_ADDRESS)
		goto load;

	if (data->initrd_file && !strcmp(data->initrd_file, "/dev/ramload")) {
		fd = open("/dev/ramload", O_RDONLY);
		if (fd >= 0) {
			mem = memmap(fd, PROT_READ);
			if (mem != MAP_FAILED)
				data->initrd_address = (u32)mem;

			close(fd);
		}
	}

	if (data->initrd_address == UIMAGE_INVALID_ADDRESS)
		data->initrd_address = PAGE_ALIGN(data->os_res->end);		

load:
	return bootm_load_initrd(data, data->initrd_address);
}

static int do_bootm_linux(struct image_data *data)
{
	void	(*kernel)(void *, void *, unsigned long,
			unsigned long, unsigned long);
	int ret;
	resource_size_t oftree_load;

	if (data->os_address == UIMAGE_INVALID_ADDRESS) {
		unsigned long base;

		if (sdram_base(&base))
			return -EINVAL;

		data->os_address = base;
	}

	ret = bootm_load_os(data, data->os_address);
	if (ret)
		return ret;

	if (bootm_has_initrd(data))
		ret = do_bootm_initrd(data);


	oftree_load = PAGE_ALIGN(data->os_res->end);
	ret = bootm_load_devicetree(data, oftree_load);
	if (ret)
		return ret;

	if (!data->oftree) {
		pr_err("bootm: No devicetree given.\n");
		return -EINVAL;
	}

	if (data->dryrun)
		return 0;

	/* Relocate the device tree if outside the initial
	 * Linux mapped TLB.
	 */
	if (IS_ENABLED(CONFIG_MPC85xx)) {
		void *addr = data->oftree;

		if ((addr + data->oftree->totalsize) > LINUX_TLB1_MAX_ADDR) {
			addr = (void *)data->os_address;

			if (bootm_relocate_fdt(addr, data))
				goto error;
		}
	}

	fdt_add_reserve_map(data->oftree);

	kernel = (void *)(data->os_address + data->os_entry);

	/* Last chance for the user to change his mind */
	if (ctrlc())
		return -1;

	devices_shutdown();
	/*
	 * Linux Kernel Parameters (passing device tree):
	 *   r3: ptr to OF flat tree, followed by the board info data
	 *   r4: physical pointer to the kernel itself
	 *   r5: NULL
	 *   r6: NULL
	 *   r7: NULL
	 */
	kernel(data->oftree, kernel, 0, 0, 0);

	restart_machine();

error:
	return -1;
}

static int do_bootm_fit(struct image_data *data)
{
	int err;

	err = fit_prepare(data);
	if (err)
		return err;

	return do_bootm_linux(data);
}

static struct image_handler handler = {
	.name = "PowerPC Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static struct image_handler handler_raw = {
	.name = "PowerPC Linux (raw)",
	.bootm = do_bootm_linux,
	.filetype = filetype_unknown,
};

static struct image_handler handler_fit = {
	.name = "Flattened Image Tree",
	.bootm = do_bootm_fit,
	.filetype = filetype_fit,
};

static int ppclinux_register_image_handler(void)
{
	register_image_handler(&handler);
	register_image_handler(&handler_raw);
	register_image_handler(&handler_fit);
	return 0;
}

late_initcall(ppclinux_register_image_handler);
