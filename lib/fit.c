/*
 * flattened image tree (fit) library functions
 *
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 *
 * Copyright (c) 2014 Westermo Teleindustri AB
 */
/* #define DEBUG */
#define pr_fmt(fmt) "fit: " fmt

#include <boot.h>
#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <libbb.h>
#include <libfdt.h>
#include <malloc.h>
#include <memory.h>
#include <uncompress.h>
#include <crc.h>
#include <libfile.h>
#include <image.h>
#include <bootm.h>

struct fit_context {
	int         fd;
	const void *fit;

	int config;

	struct image_data *image_data;

};

struct fit_part {
	struct fit_context *ctx;

	int ignore_load:1;
	int ignore_comp:1;

	const char *name;
	u32         append;
	u32         load;
	struct resource **res;
};

static int uimage_part_num(const char *partname)
{
	if (!partname)
		return 0;
	return simple_strtoul(partname, NULL, 0);
}

int fit_looks_ok(const void *fit)
{
	int images, configs;

	if (fdt_check_header(fit))
		return 0;

	images  = fdt_path_offset(fit, "/images");
	configs = fdt_path_offset(fit, "/configurations");

	/* fit only points to the first chunk of the tree, so settle
	 * for seeing one of the signature nodes. */
	return (images >= 0 || configs >= 0);
}

static int fit_default_config_offset(const void *fit)
{
	int configs;
	const char *defconf;

	configs = fdt_path_offset(fit, "/configurations");
	if (configs < 0)
		return configs;

	defconf = fdt_getprop(fit, configs, "default", NULL);
	if (!defconf || !*defconf)
		return -ENOENT;

	return fdt_subnode_offset(fit, configs, defconf);
}

static int fit_config_offset(const void *fit, int id)
{
	int configs;
	char name[16];

	configs = fdt_path_offset(fit, "/configurations");
	if (configs < 0)
		return configs;

	snprintf(name, sizeof(name), "config@%x", id);
	return fdt_subnode_offset(fit, configs, name);
}

static int fit_image_offset(const void *fit, const char *name)
{
	int images = fdt_path_offset(fit, "/images");

	if (images < 0)
		return images;

	return fdt_subnode_offset(fit, images, name);
}

static int fit_part_verify(const void *fit, int offset, const void *data, int sz)
{
	/* multiple hashes may exist, take the first one for now. */
	int hash = fdt_subnode_offset(fit, offset, "hash");
	const char *algo;
	const void *value;

	pr_info("  verifying %p, size %#.8x\n", data, (unsigned)sz);

	/* no hash here to verify -> ok */
	if (hash < 0)
		return 0;

	algo  = fdt_getprop(fit, hash, "algo", NULL);
	value = fdt_getprop(fit, hash, "value", NULL);
	if (!algo || !value)
		return -EINVAL;

	pr_info("    found hash (%s)\n", algo);

	if (!strcmp("crc32", algo)) {
		u32 crc = crc32(0, data, sz);
		u32 exp = fdt32_to_cpu(*(u32 *)value);
		pr_info("      expected:%#.8x was:%#.8x\n", exp, crc);
		return (crc == exp) ? 0 : -EINVAL;
	}
	/* add algo handlers here */

	return -EINVAL;
}

static int fit_prepare_compressed_part(struct fit_part *part, int offset,
				       const char *comp)
{
	struct fit_context *ctx = part->ctx;
	const void *data;
	void *in_ram;
	int err, sz;

	pr_info("  compressed image (%s)\n", comp);

	data = fdt_getprop(ctx->fit, offset, "data", &sz);
	if (!data)
		return -ENOENT;

	in_ram = malloc(sz);
	if (in_ram) {
		pr_info("    relocating from %p to %p, size:%#.8x\n",
			 data, in_ram, (unsigned)sz);
		err = memcpy_interruptible(in_ram, data, sz);
		if (err)
			goto err_free;

		data = in_ram;
	}

	err = fit_part_verify(ctx->fit, offset, data, sz);
	if (err)
		goto err_free;

	pr_info("    uncompressing\n");
	err = uncompress((void *)data, sz, NULL, NULL, (char *)part->load,
			 NULL, uncompress_err_stdout);

err_free:
	if (in_ram)
		free(in_ram);

	pr_info("  %s\n", err? "ERROR" : "ok");
	return err;
}

static int fit_prepare_part(struct fit_part *part)
{
	struct fit_context *ctx = part->ctx;
	const void *data, *load;
	const char *name, *comp;
	int offset, sz, err;

	pr_info("preparing %s\n", part->name);

	name   = fdt_getprop(ctx->fit, ctx->config, part->name, NULL);
	offset = fit_image_offset(ctx->fit, name);
	if (offset < 0)
		return -ENOENT;

	pr_info("  found %s\n", name);

	pr_info("  determining load addr, supplied:%#.8x append:%#.8x\n",
		 part->load, part->append);

	if (part->load == UIMAGE_INVALID_ADDRESS) {
		load = fdt_getprop(ctx->fit, offset, "load", &sz);
		if (part->ignore_load || !load || sz != sizeof(u32))
			part->load = part->append;
		else
			part->load = fdt32_to_cpu(*(u32 *)load);
	}

	pr_info("  to load at %#.8x\n", part->load);

	data = fdt_getprop(ctx->fit, offset, "data", &sz);
	if (!data)
		return -ENOENT;

	pr_info("  data at %p, size %#.8x\n", data, (unsigned)sz);

	/* TODO: is there some way to know the uncompressed size here?*/
	*part->res = request_sdram_region(part->name, part->load, sz);
	if (!*part->res)
		return -ENOMEM;

	comp = fdt_getprop(ctx->fit, offset, "compression", NULL);
	if (!part->ignore_comp && comp && strcmp("none", comp))
		return fit_prepare_compressed_part(part, offset, comp);

	pr_info("  loading image\n");

	err = memcpy_interruptible((void *)part->load, data, sz);
	if (!err)
		err = fit_part_verify(ctx->fit, offset, (void *)part->load, sz);

	if (err) {
		release_sdram_region(*part->res);
		*part->res = NULL;
	}

	pr_info("  %s\n", err? "ERROR" : "ok");
	return err;
}

int fit_prepare_kernel(struct fit_context *ctx)
{
	struct image_data *id = ctx->image_data;
	struct fit_part part = {
		.ctx    = ctx,
		.name   = "kernel",
		.load   = id->os_address,
		.res    = &id->os_res,
	};
	int err;

	err = sdram_base((unsigned long *)&part.append);
	if (err)
		return err;

	err = fit_prepare_part(&part);
	if (err)
		return err;

	id->os_address = part.load;
	return bootm_load_os(id, part.load);
}

int fit_prepare_ramdisk(struct fit_context *ctx)
{
	struct image_data *id = ctx->image_data;
	struct fit_part part = {
		.ctx  = ctx,
		.name = "ramdisk",

		.ignore_load = 1,
		.ignore_comp = 1,

		.load = id->initrd_address,
		.res  = &id->initrd_res,
	};
	int err;

	if (id->os_res)
		part.append = __ALIGN_MASK(id->os_res->end, 0x00ffffff);

	err = fit_prepare_part(&part);
	if (err)
		return err;

	id->initrd_address = part.load;
	return bootm_load_initrd(id, part.load);
}

int fit_prepare_fdt(struct fit_context *ctx)
{
	struct image_data *id = ctx->image_data;
	const void *data;
	const char *name;
	int offset;
	u32 load = UIMAGE_INVALID_ADDRESS;

	pr_info("preparing device tree\n");

	name   = fdt_getprop(ctx->fit, ctx->config, "fdt", NULL);
	offset = fit_image_offset(ctx->fit, name);
	if (offset < 0)
		return -ENOENT;

	pr_info("  found %s\n", name);

	data = fdt_getprop(ctx->fit, offset, "data", NULL);
	if (!data)
		return -ENOENT;

	pr_info("  data at %p\n", data);

	if (id->os_res)
		load = PAGE_ALIGN(id->os_res->end);

	if (id->initrd_res && id->initrd_res->end > load)
		load = PAGE_ALIGN(id->initrd_res->end);

	id->of_root_node = of_unflatten_dtb((void *)data);
	return bootm_load_devicetree(id, load);
}

int fit_prepare(struct image_data *data)
{
	struct fit_context *ctx = xzalloc(sizeof(*ctx));
	int err = -EINVAL;
	int num;

	ctx->image_data = data;

	ctx->fd = open(data->os_file, O_RDONLY);
	if (ctx->fd < 0) {
		err = ctx->fd;
		goto err_free;
	}

	ctx->fit = memmap(ctx->fd, PROT_READ);
	if (ctx->fit == (void *)-1)
		goto err_close;

	if (!fit_looks_ok(ctx->fit))
		goto err_close;

	num = uimage_part_num(data->os_part);

	if (num)
		ctx->config = fit_config_offset(ctx->fit, num);
	else
		ctx->config = fit_default_config_offset(ctx->fit);

	if (ctx->config < 0)
		goto err_close;

	err = fit_prepare_kernel(ctx);
	if (err)
		goto err_close;

	err = fit_prepare_ramdisk(ctx);
	if (err)
		goto err_close;

	err = fit_prepare_fdt(ctx);
	if (err)
		goto err_close;

err_close:
	close(ctx->fd);
err_free:
	free(ctx);

	pr_info("%s\n", err ? "ERROR" : "ok");
	return err;
}
