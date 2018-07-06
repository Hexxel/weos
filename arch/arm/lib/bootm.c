#include <bootm.h>
#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <libfile.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/sizes.h>
#include <libbb.h>
#include <magicvar.h>
#include <binfmt.h>
#include <restart.h>
#include <glob.h>
#include <globalvar.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

#include <crc.h>
#include <cramfs/cramfs_fs.h>
#include <../fs/squashfs/squashfs_fs.h>

#include <generated/utsrelease.h>


/*
 * sdram_start_and_size() - determine place for putting the kernel/oftree/initrd
 *
 * @start:	returns the start address of the first RAM bank
 * @size:	returns the usable space at the beginning of the first RAM bank
 *
 * This function returns the base address of the first RAM bank and the free
 * space found there.
 *
 * return: 0 for success, negative error code otherwise
 */
static int sdram_start_and_size(unsigned long *start, unsigned long *size)
{
	struct memory_bank *bank;
	struct resource *res;

	/*
	 * We use the first memory bank for the kernel and other resources
	 */
	bank = list_first_entry_or_null(&memory_banks, struct memory_bank,
			list);
	if (!bank) {
		printf("cannot find first memory bank\n");
		return -EINVAL;
	}

	/*
	 * If the first memory bank has child resources we can use the bank up
	 * to the beginning of the first child resource, otherwise we can use
	 * the whole bank.
	 */
	res = list_first_entry_or_null(&bank->res->children, struct resource,
			sibling);
	if (res)
		*size = res->start - bank->start;
	else
		*size = bank->size;

	*start = bank->start;

	return 0;
}

static int get_kernel_addresses(size_t image_size,
				 int verbose, unsigned long *load_address,
				 unsigned long *mem_free)
{
	unsigned long mem_start, mem_size;
	int ret;
	size_t image_decomp_size;
	unsigned long spacing;

	ret = sdram_start_and_size(&mem_start, &mem_size);
	if (ret)
		return ret;

	/*
	 * The kernel documentation "Documentation/arm/Booting" advises
	 * to place the compressed image outside of the lowest 32 MiB to
	 * avoid relocation. We should do this if we have at least 64 MiB
	 * of ram. If we have less space, we assume a maximum
	 * compression factor of 5.
	 */
	image_decomp_size = PAGE_ALIGN(image_size * 5);
	if (mem_size >= SZ_64M)
		image_decomp_size = max_t(size_t, image_decomp_size, SZ_32M);

	/*
	 * By default put oftree/initrd close behind compressed kernel image to
	 * avoid placing it outside of the kernels lowmem region.
	 */
	spacing = SZ_1M;

	if (*load_address == UIMAGE_INVALID_ADDRESS) {
		/*
		 * Place the kernel at an address where it does not need to
		 * relocate itself before decompression.
		 */
		*load_address = mem_start + image_decomp_size;
		if (verbose)
			printf("no OS load address, defaulting to 0x%08lx\n",
				*load_address);
	} else if (*load_address <= mem_start + image_decomp_size) {
		/*
		 * If the user/image specified an address where the kernel needs
		 * to relocate itself before decompression we need to extend the
		 * spacing to allow this relocation to happen without
		 * overwriting anything placed behind the kernel.
		 */
		spacing += image_decomp_size;
	}

	*mem_free = PAGE_ALIGN(*load_address + image_size + spacing);

	/*
	 * Place oftree/initrd outside of the first 128 MiB, if we have space
	 * for it. This avoids potential conflicts with the kernel decompressor.
	 */
	if (mem_size > SZ_256M)
		*mem_free = max(*mem_free, mem_start + SZ_128M);

	return 0;
}

static int __do_bootm_linux(struct image_data *data, unsigned long free_mem, int swap)
{
	unsigned long kernel;
	unsigned long initrd_start = 0, initrd_size = 0, initrd_end = 0;
	enum arm_security_state state = bootm_arm_security_state();
	int ret;

	kernel = data->os_res->start + data->os_entry;

	initrd_start = data->initrd_address;

	if (initrd_start == UIMAGE_INVALID_ADDRESS) {
		initrd_start = PAGE_ALIGN(free_mem);

		if (bootm_verbose(data)) {
			printf("no initrd load address, defaulting to 0x%08lx\n",
				initrd_start);
		}
	}

	if (bootm_has_initrd(data)) {
		ret = bootm_load_initrd(data, initrd_start);
		if (ret)
			return ret;
	}

	if (data->initrd_res) {
		initrd_start = data->initrd_res->start;
		initrd_end = data->initrd_res->end;
		initrd_size = resource_size(data->initrd_res);
		if (initrd_end > free_mem)
			free_mem = PAGE_ALIGN(initrd_end);
	}

	ret = bootm_load_devicetree(data, free_mem);
	if (ret)
		return ret;

	if (bootm_verbose(data)) {
		printf("\nStarting kernel at 0x%08lx", kernel);
		if (initrd_size)
			printf(", initrd at 0x%08lx", initrd_start);
		if (data->oftree)
			printf(", oftree at 0x%p", data->oftree);
		printf("...\n");
	}

	if (IS_ENABLED(CONFIG_ARM_SECURE_MONITOR)) {
		if (file_detect_type((void *)data->os_res->start, 0x100) ==
		    filetype_arm_barebox)
			state = ARM_STATE_SECURE;

		printf("Starting kernel in %s mode\n",
		       bootm_arm_security_state_name(state));
	}

	if (data->dryrun)
		return 0;

	start_linux((void *)kernel, swap, initrd_start, initrd_size, data->oftree,
		    state);

	restart_machine();

	return -ERESTARTSYS;
}

static int do_bootm_linux(struct image_data *data)
{
	unsigned long load_address, mem_free;
	int ret;

	load_address = data->os_address;

	ret = get_kernel_addresses(bootm_get_os_size(data),
			     bootm_verbose(data), &load_address, &mem_free);
	if (ret)
		return ret;

	ret = bootm_load_os(data, load_address);
	if (ret)
		return ret;

	devices_shutdown();

	return __do_bootm_linux(data, mem_free, 0);
}

static struct image_handler uimage_handler = {
	.name = "ARM Linux uImage",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static struct image_handler rawimage_handler = {
	.name = "ARM raw image",
	.bootm = do_bootm_linux,
	.filetype = filetype_unknown,
};

struct zimage_header {
	u32	unused[9];
	u32	magic;
	u32	start;
	u32	end;
};

#define ZIMAGE_MAGIC 0x016F2818

static int do_bootz_linux_fdt(int fd, struct image_data *data)
{
	struct fdt_header __header, *header;
	void *oftree;
	int ret;

	u32 end;

	if (data->oftree)
		return -ENXIO;

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < 0)
		return ret;
	if (ret < sizeof(*header))
		return -ENXIO;

	if (file_detect_type(header, sizeof(*header)) != filetype_oftree)
		return -ENXIO;

	end = be32_to_cpu(header->totalsize);

	oftree = malloc(end + 0x8000);
	if (!oftree) {
		perror("zImage: oftree malloc");
		return -ENOMEM;
	}

	memcpy(oftree, header, sizeof(*header));

	end -= sizeof(*header);

	ret = read_full(fd, oftree + sizeof(*header), end);
	if (ret < 0)
		goto err_free;
	if (ret < end) {
		printf("premature end of image\n");
		ret = -EIO;
		goto err_free;
	}

	if (IS_BUILTIN(CONFIG_OFTREE)) {
		struct device_node *root;

		root = of_unflatten_dtb(oftree);
		if (IS_ERR(root)) {
			pr_err("unable to unflatten devicetree\n");
			goto err_free;
		}
		data->oftree = of_get_fixed_tree(root);
		if (!data->oftree) {
			pr_err("Unable to get fixed tree\n");
			ret = -EINVAL;
			goto err_free;
		}

		free(oftree);
	} else {
		data->oftree = oftree;
	}

	pr_info("zImage: concatenated oftree detected\n");

	return 0;

err_free:
	free(oftree);

	return ret;
}

static int do_bootz_linux(struct image_data *data)
{
	int fd, ret, swap = 0;
	struct zimage_header __header, *header;
	void *zimage;
	u32 end, start;
	size_t image_size;
	unsigned long load_address = data->os_address;
	unsigned long mem_free;

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < sizeof(*header)) {
		printf("could not read %s\n", data->os_file);
		goto err_out;
	}

	switch (header->magic) {
	case swab32(ZIMAGE_MAGIC):
		swap = 1;
		/* fall through */
	case ZIMAGE_MAGIC:
		break;
	default:
		printf("invalid magic 0x%08x\n", header->magic);
		ret = -EINVAL;
		goto err_out;
	}

	end = header->end;
	start = header->start;

	if (swap) {
		end = swab32(end);
		start = swab32(start);
	}

	image_size = end - start;
	load_address = data->os_address;

	ret = get_kernel_addresses(image_size, bootm_verbose(data),
			     &load_address, &mem_free);
	if (ret)
		return ret;

	data->os_res = request_sdram_region("zimage", load_address, image_size);
	if (!data->os_res) {
		pr_err("bootm/zImage: failed to request memory at 0x%lx to 0x%lx (%d).\n",
		       load_address, load_address + image_size, image_size);
		ret = -ENOMEM;
		goto err_out;
	}

	zimage = (void *)data->os_res->start;

	memcpy(zimage, header, sizeof(*header));

	ret = read_full(fd, zimage + sizeof(*header),
			image_size - sizeof(*header));
	if (ret < 0)
		goto err_out;
	if (ret < image_size - sizeof(*header)) {
		printf("premature end of image\n");
		ret = -EIO;
		goto err_out;
	}

	if (swap) {
		void *ptr;
		for (ptr = zimage; ptr < zimage + end; ptr += 4)
			*(u32 *)ptr = swab32(*(u32 *)ptr);
	}

	ret = do_bootz_linux_fdt(fd, data);
	if (ret && ret != -ENXIO)
		goto err_out;

	close(fd);

	return __do_bootm_linux(data, mem_free, swap);

err_out:
	close(fd);

	return ret;
}

static struct image_handler zimage_handler = {
	.name = "ARM zImage",
	.bootm = do_bootz_linux,
	.filetype = filetype_arm_zimage,
};

static struct image_handler barebox_handler = {
	.name = "ARM barebox",
	.bootm = do_bootm_linux,
	.filetype = filetype_arm_barebox,
};

#include <aimage.h>

static int aimage_load_resource(int fd, struct resource *r, void* buf, int ps)
{
	int ret;
	void *image = (void *)r->start;
	unsigned to_read = ps - resource_size(r) % ps;

	ret = read_full(fd, image, resource_size(r));
	if (ret < 0)
		return ret;

	ret = read_full(fd, buf, to_read);
	if (ret < 0)
		printf("could not read dummy %u\n", to_read);

	return ret;
}

static int do_bootm_aimage(struct image_data *data)
{
	struct resource *snd_stage_res;
	int fd, ret;
	struct android_header __header, *header;
	void *buf;
	int to_read;
	struct android_header_comp *cmp;
	unsigned long mem_free;
	unsigned long mem_start, mem_size;

	ret = sdram_start_and_size(&mem_start, &mem_size);
	if (ret)
		return ret;

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < sizeof(*header)) {
		printf("could not read %s\n", data->os_file);
		goto err_out;
	}

	printf("Android Image for '%s'\n", header->name);

	/*
	 * As on tftp we do not support lseek and we will just have to seek
	 * for the size of a page - 1 max just buffer instead to read to dummy
	 * data
	 */
	buf = xmalloc(header->page_size);

	to_read = header->page_size - sizeof(*header);
	ret = read_full(fd, buf, to_read);
	if (ret < 0) {
		printf("could not read dummy %d from %s\n", to_read, data->os_file);
		goto err_out;
	}

	cmp = &header->kernel;
	data->os_res = request_sdram_region("akernel", cmp->load_addr, cmp->size);
	if (!data->os_res) {
		pr_err("Cannot request region 0x%08x - 0x%08x, using default load address\n",
				cmp->load_addr, cmp->size);

		data->os_address = mem_start + PAGE_ALIGN(cmp->size * 4);
		data->os_res = request_sdram_region("akernel", data->os_address, cmp->size);
		if (!data->os_res) {
			pr_err("Cannot request region 0x%08x - 0x%08x\n",
					cmp->load_addr, cmp->size);
			ret = -ENOMEM;
			goto err_out;
		}
	}

	ret = aimage_load_resource(fd, data->os_res, buf, header->page_size);
	if (ret < 0) {
		perror("could not read kernel");
		goto err_out;
	}

	/*
	 * fastboot always expect a ramdisk
	 * in barebox we can be less restrictive
	 */
	cmp = &header->ramdisk;
	if (cmp->size) {
		data->initrd_res = request_sdram_region("ainitrd", cmp->load_addr, cmp->size);
		if (!data->initrd_res) {
			ret = -ENOMEM;
			goto err_out;
		}

		ret = aimage_load_resource(fd, data->initrd_res, buf, header->page_size);
		if (ret < 0) {
			perror("could not read initrd");
			goto err_out;
		}
	}

	if (!getenv("aimage_noverwrite_bootargs"))
		linux_bootargs_overwrite(header->cmdline);

	if (!getenv("aimage_noverwrite_tags"))
		armlinux_set_bootparams((void*)header->tags_addr);

	cmp = &header->second_stage;
	if (cmp->size) {
		void (*second)(void);

		snd_stage_res = request_sdram_region("asecond", cmp->load_addr, cmp->size);
		if (!snd_stage_res) {
			ret = -ENOMEM;
			goto err_out;
		}

		ret = aimage_load_resource(fd, snd_stage_res, buf, header->page_size);
		if (ret < 0) {
			perror("could not read initrd");
			goto err_out;
		}

		second = (void*)snd_stage_res->start;
		shutdown_barebox();

		second();

		restart_machine();
	}

	close(fd);

	/*
	 * Put devicetree right after initrd if present or after the kernel
	 * if not.
	 */
	if (data->initrd_res)
		mem_free = PAGE_ALIGN(data->initrd_res->end);
	else
		mem_free = PAGE_ALIGN(data->os_res->end + SZ_1M);

	return __do_bootm_linux(data, mem_free, 0);

err_out:
	linux_bootargs_overwrite(NULL);
	close(fd);

	return ret;
}

static struct image_handler aimage_handler = {
	.name = "ARM Android Image",
	.bootm = do_bootm_aimage,
	.filetype = filetype_aimage,
};

#ifdef CONFIG_BOOTM_AIMAGE
BAREBOX_MAGICVAR(aimage_noverwrite_bootargs, "Disable overwrite of the bootargs with the one present in aimage");
BAREBOX_MAGICVAR(aimage_noverwrite_tags, "Disable overwrite of the tags addr with the one present in aimage");
#endif

static struct image_handler arm_fit_handler = {
        .name = "FIT image",
        .bootm = do_bootm_linux,
        .filetype = filetype_oftree,
};

static struct binfmt_hook binfmt_aimage_hook = {
	.type = filetype_aimage,
	.exec = "bootm",
};

static struct binfmt_hook binfmt_arm_zimage_hook = {
	.type = filetype_arm_zimage,
	.exec = "bootm",
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_arm_barebox,
	.exec = "bootm",
};


static int __cramfs_crc(const struct cramfs_super *img)
{
	struct cramfs_super hdr = *img;
	u32 crc, img_crc = CRAMFS_32(img->fsid.crc);

	hdr.fsid.crc = 0;
	crc = crc32(0, &hdr, sizeof(hdr));

	crc = crc32(crc, img + 1, CRAMFS_32(img->size) - sizeof (*img));

	if (crc != img_crc)
		pr_info("bootm: invalid CRC\n");

	return crc != img_crc;
}

static void __add_mtdparts_one(char *mtdparts, const char *flash)
{
	char *part, *flashdir = basprintf("/env/etc/mtdparts/%s/*", flash);
	glob_t g;
	int err, i;

	err = glob(flashdir, 0, NULL, &g);
	if (err == GLOB_NOMATCH)
		return;

	strcat(mtdparts, flash);
	strcat(mtdparts, ":");

	for (i = 0; i < g.gl_pathc; i++) {
		if (i)
			strcat(mtdparts, ",");

		part = read_file_line(g.gl_pathv[i]);
		strcat(mtdparts, part);
	}

	globfree(&g);
}

static void __add_mtdparts(void)
{
	char *mtdparts = xzalloc(1024);
	struct dirent *flash;
	DIR *dir;
	int first = 1;

	dir = opendir("/env/etc/mtdparts");
	if (!dir)
		return;

	strcpy(mtdparts, "mtdparts=");

	while ((flash = readdir(dir))) {
		if (DOT_OR_DOTDOT(flash->d_name))
			continue;

		if (first)
			first = 0;
		else
			strcat(mtdparts, ";");

		__add_mtdparts_one(mtdparts, flash->d_name);
	}

	closedir(dir);

	globalvar_add_simple("linux.bootargs.mtdparts", mtdparts);
}

static int do_bootm_cramfs(struct image_data *data)
{
	struct cramfs_super *img;
	unsigned long crc, imgsz, ksz;
	int fd;

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0)
		return fd;

	img = memmap(fd, PROT_READ);
	close(fd);
	if (!img)
		return -EIO;

	if (img == -1)
		return -EIO;

	crc   = CRAMFS_32(img->fsid.crc);
	imgsz = CRAMFS_32(img->size);
	ksz   = CRAMFS_GET_OFFSET (&(img->root)) << 2;

	if (data->initrd_file) {
		if (img != (void *)data->initrd_address)
			return -EINVAL;

		data->initrd_res = request_sdram_region("initrd",
							data->initrd_address,
							imgsz);
		if (!data->initrd_res)
			return -ENOMEM;

		data->os_address = PAGE_ALIGN(data->initrd_res->end);

		globalvar_add_simple("linux.bootargs.rdsize",
				     basprintf("ramdisk_size=%lu",
					      (imgsz + SZ_1K - 1) / SZ_1K));
	}

	data->os_res = request_sdram_region("zImage", data->os_address, ksz);
	if (!data->os_res)
		return -ENOMEM;

	memcpy((void *)data->os_address, img + 1, ksz);

	if (__cramfs_crc(img))
		return -EINVAL;

	__add_mtdparts();

	globalvar_add_simple("linux.bootargs.redboot_rel",
			     basprintf("redboot_rel=%s", UTS_RELEASE));

	printf("\e[;1m[ OK ]\e[0m\n");
	return __do_bootm_linux(data, 0, 0);
}

static struct image_handler cramfs_handler = {
	.name = "Westermo CramFS Image",
	.bootm = do_bootm_cramfs,
	.filetype = filetype_cramfs,
};

static struct binfmt_hook binfmt_cramfs_hook = {
	.type = filetype_cramfs,
	.exec = "bootm",
};

static int do_bootm_squashfs(struct image_data *data)
{
	struct squashfs_super_block *img;
	unsigned long imgsz;
	int fd;

	if (!data->initrd_file)
		goto bootz;

	fd = open(data->initrd_file, O_RDONLY);
	if (fd < 0)
		return fd;

	img = memmap(fd, PROT_READ);
	close(fd);
	if (!img)
		return -EIO;

	if (img != (void *)data->initrd_address)
		return -EINVAL;

	imgsz = le64_to_cpu(img->bytes_used);
	data->initrd_res = request_sdram_region("initrd",
						data->initrd_address,
						imgsz);
	if (!data->initrd_res)
		return -ENOMEM;

	data->os_address = PAGE_ALIGN(data->initrd_res->end);

	globalvar_add_simple("linux.bootargs.rdsize",
			     basprintf("ramdisk_size=%lu",
				       (imgsz + SZ_1K - 1) / SZ_1K));
bootz:
	__add_mtdparts();

	return do_bootz_linux(data);
}

static struct image_handler squashfs_handler = {
	.name = "Westermo SquashFS Image",
	.bootm = do_bootm_squashfs,
	.filetype = filetype_arm_zimage,
};

static int armlinux_register_image_handler(void)
{
	register_image_handler(&cramfs_handler);
	register_image_handler(&squashfs_handler);

	register_image_handler(&barebox_handler);
	register_image_handler(&uimage_handler);
	register_image_handler(&rawimage_handler);
	register_image_handler(&zimage_handler);
	if (IS_BUILTIN(CONFIG_BOOTM_AIMAGE)) {
		register_image_handler(&aimage_handler);
		binfmt_register(&binfmt_aimage_hook);
	}
	if (IS_BUILTIN(CONFIG_FITIMAGE))
	        register_image_handler(&arm_fit_handler);
	binfmt_register(&binfmt_arm_zimage_hook);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(armlinux_register_image_handler);
