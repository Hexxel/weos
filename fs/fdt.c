/*
 * fdt.c - a device file system for fdt files
 *
 * Copyright (c) 2014 Westermo Teleindustri AB
 *
 * Author: Tobias Waldekranz
 *
 * Based on devfs.c, original copyright follows.
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <libfdt.h>
#include <libgen.h>
#include <malloc.h>

#include <linux/stat.h>

struct fdtfs_priv {
	int   fd;
	const void *fdt;
};

struct fdtfs_inode {
	const struct fdt_property *prop;
};

static const struct fdt_property *__get_prop(const void *fdt, const char *path)
{
	int node;
	const struct fdt_property *prop = NULL;
	char *pathdup = strdup(path);
	char *dname = dirname(pathdup);

	node = fdt_path_offset(fdt, dname);
	if (node >= 0)
		prop = fdt_get_property(fdt, node, basename((char *)path), NULL);

	free(pathdup);
	return prop;
}


static int fdtfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct fdtfs_inode *inode = f->priv;
	const char *src = inode->prop->data + f->pos;

	memcpy(buf, src, size);
	return size;
}

static loff_t fdtfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;
	return pos;
}

static int fdtfs_open(struct device_d *dev, FILE *f, const char *path)
{
	struct fdtfs_priv *priv = dev->priv;
	struct fdtfs_inode *inode = xzalloc(sizeof(*inode));
	int err = -ENOENT;

	inode->prop = __get_prop(priv->fdt, path);
	if (!inode->prop)
		goto err_free;

	f->size = inode->prop->len;
	f->priv = inode;
	return 0;

err_free:
	free(inode);
	return err;
}

static int fdtfs_close(struct device_d *dev, FILE *f)
{
	struct fdtfs_inode *inode = f->priv;

	free(inode);
	return 0;
}

static DIR* fdtfs_opendir(struct device_d *dev, const char *path)
{
	char spath[PATH_MAX] = { '/', '\0' };
	struct fdtfs_priv *priv = dev->priv;
	int node, next;
	DIR *dir;

	if (path[0] != '/')
		path = strncat(spath, path, sizeof(spath));

	node = fdt_path_offset(priv->fdt, path);
	if (node < 0)
		return NULL;

	fdt_next_tag(priv->fdt, node, &next);

	dir = xzalloc(sizeof(*dir));
	dir->priv = (void *)next;
	return dir;
}

static int __skip_subtree(const void *fdt, int node)
{
	int tag, next, depth = 1;

	while (depth) {
		tag = fdt_next_tag(fdt, node, &next);

		if (tag == FDT_BEGIN_NODE)
			depth++;
		else if (tag == FDT_END_NODE)
			depth--;

		node = next;
	}

	return node;
}

static struct dirent* fdtfs_readdir(struct device_d *dev, DIR *dir)
{
	struct fdtfs_priv *priv = dev->priv;
	int tag, next, node = (int)dir->priv;
	const struct fdt_property *prop;
	const char *name;

	tag = fdt_next_tag(priv->fdt, node, &next);
	switch (tag) {
	case FDT_BEGIN_NODE:
		name = fdt_get_name(priv->fdt, node, NULL);
		next = __skip_subtree(priv->fdt, next);
		break;

	case FDT_PROP:
		prop = fdt_get_property_by_offset(priv->fdt, node, NULL);
		name = fdt_string(priv->fdt, fdt32_to_cpu(prop->nameoff));
		break;

	case FDT_NOP:
		name = "<NOP>";
		break;

	case FDT_END_NODE:
		return NULL;

	default:
		pr_err("readdir: unexpected tag (%d)\n", tag);
		return NULL;
	}

	strcpy(dir->d.d_name, name);
	dir->priv = (void *)next;
	return &dir->d;
}

static int fdtfs_closedir(struct device_d *dev, DIR *dir)
{
	free(dir);
	return 0;
}

static int __stat_node(const void *fdt, const char *path, struct stat *s)
{
	int node;

	node = fdt_path_offset(fdt, path);
	if (node < 0)
		return -ENOENT;

	s->st_mode = S_IFDIR | 0555;
	s->st_size = 0;
	return 0;
}

static int __stat_prop(const void *fdt, const char *path, struct stat *s)
{
	const struct fdt_property *prop = __get_prop(fdt, path);

	if (!prop)
		return -ENOENT;

	s->st_mode = S_IFREG | 0444;
	s->st_size = prop->len;
	return 0;
}

static int fdtfs_stat(struct device_d *dev, const char *path, struct stat *s)
{
	struct fdtfs_priv *priv = dev->priv;

	if (!__stat_node(priv->fdt, path, s))
		return 0;

	return __stat_prop(priv->fdt, path, s); 
}

static int fdtfs_probe(struct device_d *dev)
{
	struct fs_device_d *fdev = dev_to_fs_device(dev);
	struct fdtfs_priv *priv = xzalloc(sizeof(*priv));
	int err = -EINVAL;

	dev->priv = priv;

	priv->fd = open(fdev->backingstore, O_RDONLY);
	if (priv->fd < 0) {
		err = priv->fd;
		goto err_free;
	}

	priv->fdt = memmap(priv->fd, PROT_READ);
	if (priv->fdt == (void *)-1) 
		goto err_close;

	if (fdt_check_header(priv->fdt))
		goto err_close;

	return 0;

err_close:
	close(priv->fd);
err_free:
	free(priv);
	return err;
}

static void fdtfs_delete(struct device_d *dev)
{
	struct fdtfs_priv *priv = dev->priv;

	close(priv->fd);
	free(priv);
}

static struct fs_driver_d fdtfs_driver = {
	.read      = fdtfs_read,
	.lseek     = fdtfs_lseek,
	.open      = fdtfs_open,
	.close     = fdtfs_close,
	.opendir   = fdtfs_opendir,
	.readdir   = fdtfs_readdir,
	.closedir  = fdtfs_closedir,
	.stat      = fdtfs_stat,
	.drv = {
		.probe  = fdtfs_probe,
		.remove = fdtfs_delete,
		.name = "fdt",
	}
};

static int fdtfs_init(void)
{
	return register_fs_driver(&fdtfs_driver);
}

coredevice_initcall(fdtfs_init);
