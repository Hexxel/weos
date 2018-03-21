/* #include <asm/uaccess.h> */

/* #include <linux/device.h> */
/* #include <linux/fs.h> */
/* #include <linux/io.h> */
/* #include <linux/kernel.h> */
/* #include <linux/module.h> */
/* #include <linux/slab.h> */

#include <common.h>
#include <driver.h>

#include "xc3_private.h"

/* static loff_t xc3_cdev_lseek(struct cdev *cdev, loff_t offset) */
/* { */
/* 	loff_t cur = filp->f_pos; */

/* 	filp->f_pos = cur; */
/* 	return cur; */
/* } */

static ssize_t xc3_cdev_read(struct cdev *cdev, void *buf,
			     size_t sz, loff_t offs, ulong flags)
{
	struct xc3 *xc3 = cdev->priv;
	size_t left;
	u32 *mem, *wbuf = buf;
	int i;

	left = ADDR_COMPL_SZ - (offs & (ADDR_COMPL_SZ - 1));
	if (sz > left)
		sz = left;

	mem = xc3_win_get(xc3, offs);

	for (i = 0; i < (sz >> 2); i++)
		wbuf[i] = readl(&mem[i]);

	xc3_win_put(xc3);

	return sz;
}

static ssize_t xc3_cdev_write(struct cdev *cdev, const void *buf,
			     size_t sz, loff_t offs, ulong flags)
{
	struct xc3 *xc3 = cdev->priv;
	size_t left;
	const u32 *wbuf = buf;
	u32 *mem;
	int i;

	left = ADDR_COMPL_SZ - (offs & (ADDR_COMPL_SZ - 1));
	if (sz > left)
		sz = left;

	mem = xc3_win_get(xc3, offs);

	for (i = 0; i < (sz >> 2); i++)
		writel(wbuf[i], &mem[i]);

	xc3_win_put(xc3);

	return sz;
}


/* static int xc3_cdev_open(struct inode *inode, struct file *filp) */
/* { */
/* 	struct xc3 *xc3 = container_of(inode->i_cdev, struct xc3, cdev.cdev); */

/* 	filp->private_data = xc3; */
	
/* 	return 0; */
/* } */

/* static int xc3_cdev_release(struct inode *inode, struct file *filp) */
/* { */
/* 	return 0; */
/* } */

static struct file_operations xc3_cdev_fops = {
	.lseek  = dev_lseek_default,
	.read    = xc3_cdev_read,
	.write   = xc3_cdev_write,
	/* .open    = xc3_cdev_open, */
	/* .release = xc3_cdev_release, */
};

int xc3_cdev_init(struct xc3 *xc3)
{
	/* dev_t devnum; */
	/* int err; */

	/* err = alloc_chrdev_region(&devnum, 0, 1, "xc3"); */
	/* if (err) */
	/* 	return err; */

	xc3->cdev.ops = &xc3_cdev_fops;
	xc3->cdev.size = 0xffffffff;
	xc3->cdev.name = "xc3";
	xc3->cdev.priv = xc3;

	return devfs_create(&xc3->cdev);

	/* cdev_init(&xc3->cdev.cdev, &xc3_cdev_fops); */
	/* xc3->cdev.cdev.owner = THIS_MODULE; */
	/* err = cdev_add(&xc3->cdev.cdev, devnum, 1); */
	/* if (err) */
	/* 	return err; */

	/* xc3->cdev.class = class_create(xc3->cdev.cdev.owner, "xc3"); */
	/* if (IS_ERR(xc3->cdev.class)) */
	/* 	return PTR_ERR(xc3->cdev.class); */

	/* xc3->cdev.dev = device_create(xc3->cdev.class, NULL, devnum, NULL, "xc3"); */
	/* if (!xc3->cdev.dev) */
	/* 	return -ENODEV; */
		
	/* return 0; */
}
