#include <asm/uaccess.h>

#include <linux/debugfs.h>

#include "xc3_private.h"

const char *tag_mode[] = {
	[XC3_VLAN_UNTAGGED]  = "u",
	[XC3_VLAN_TAG0]      = "s",
	[XC3_VLAN_TAG1]      = "c",
	[XC3_VLAN_TAG01]     = "sc",
	[XC3_VLAN_TAG10]     = "cs",
	[XC3_VLAN_TAG0_PUSH] = "pushs",
	[XC3_VLAN_POP]       = "pop"
};

static int xc3_debugfs_vlan_show(struct seq_file *s, void *data)
{
	struct xc3 *xc3 = s->private;
	struct xc3_vlt_entry entry;
	const char *m = NULL, *lastm = NULL;
	int i, p, p_start;

	for (i = 0; i < 4096; i++) {
		xc3_vlt_read(xc3, XC3_VLT_VLAN, i, &entry);
		if (!xc3_vlan_get_valid(&entry))
			continue;

		seq_printf(s, "vid:%-4d cpu:%s members:", i,
			   xc3_vlan_get_cpu_member(&entry)? "yes" : "no");
		p_start = 0;
		for (p = 0; p < 27; p++) {
			m = NULL;
			if (xc3_vlan_get_member(&entry, p))
				m = tag_mode[xc3_vlan_get_tag(&entry, p)];

			if (m == lastm)
				;
			else if (m) {
				p_start = p;
				seq_printf(s, " %d", p);
			} else {
				if (p_start == p - 1)
					seq_printf(s, "(%s)", lastm);
				else
					seq_printf(s, "-%d(%s)", p - 1, lastm);
			}

			lastm = m;
		}

		m = NULL;
		if (xc3_vlan_get_member(&entry, 27))
			m = tag_mode[xc3_vlan_get_tag(&entry, p)];

		if (!m && p_start == 26)
			seq_printf(s, "(%s)", lastm);
		else
			seq_printf(s, "-%d(%s)", m? 27 : 26, lastm);

		seq_puts(s, "\n");
	}
 
	return 0;
}

static int xc3_debugfs_vlan_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, xc3_debugfs_vlan_show, inode->i_private);
}

static const struct file_operations xc3_debugfs_vlan_fops = {
	.open		= xc3_debugfs_vlan_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int xc3_debugfs_vlan_raw_show(struct seq_file *s, void *data)
{
	struct xc3 *xc3 = s->private;
	struct xc3_vlt_entry entry;
	int i;

	for (i = 0; i < 4096; i++) {
		xc3_vlt_read(xc3, XC3_VLT_VLAN, i, &entry);
		seq_printf(s, "vid:%-4d %svalid %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x\n",
			   i, xc3_vlan_get_valid(&entry) ? "  " : "in",
			   entry.data[0], entry.data[1], entry.data[2],
			   entry.data[3], entry.data[4], entry.data[5]);
	}
 
	return 0;
}

static int xc3_debugfs_vlan_raw_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, xc3_debugfs_vlan_raw_show, inode->i_private);
}

static const struct file_operations xc3_debugfs_vlan_raw_fops = {
	.open		= xc3_debugfs_vlan_raw_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

const char *fdb_type[] = {
	[XC3_FDB_MAC]  = "mac ",
	[XC3_FDB_IPV4] = "ipv4",
	[XC3_FDB_IPV6] = "ipv6"
};

static void __print_mac(struct seq_file *s, struct xc3_fdb_entry *entry)
{
	u8 mac[6];

	xc3_fdb_get_mac(entry, mac);

	seq_printf(s, "mac:%pM port:%-2d vid:%-4d\n", mac,
		   xc3_fdb_get_port(entry), xc3_fdb_get_vid(entry));
}

static int xc3_debugfs_fdb_show(struct seq_file *s, void *data)
{
	struct xc3 *xc3 = s->private;
	struct xc3_fdb_entry entry;
	int i;

	for (i = 0; i < 16384; i++) {
		xc3_fdb_read(xc3, i, &entry);
		if (!xc3_fdb_get_valid(&entry))
			continue;

		switch (xc3_fdb_get_type(&entry)) {
		case XC3_FDB_MAC:
			__print_mac(s, &entry);
			break;

		default:
			seq_printf(s, "row:%-5d %s %8.8x %8.8x %8.8x %8.8x\n",
				   i, fdb_type[xc3_fdb_get_type(&entry)],
				   entry.data[0], entry.data[1],
				   entry.data[2], entry.data[3]);
		}

	}
 
	return 0;
}

static int xc3_debugfs_fdb_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, xc3_debugfs_fdb_show, inode->i_private);
}

static const struct file_operations xc3_debugfs_fdb_fops = {
	.open		= xc3_debugfs_fdb_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int xc3_debugfs_fdb_raw_show(struct seq_file *s, void *data)
{
	struct xc3 *xc3 = s->private;
	struct xc3_fdb_entry entry;
	int i;

	for (i = 0; i < 16384; i++) {
		xc3_fdb_read(xc3, i, &entry);
		seq_printf(s, "row:%-5d %svalid %8.8x %8.8x %8.8x %8.8x\n", i,
			   xc3_fdb_get_valid(&entry)? "  " : "in",
			   entry.data[0], entry.data[1], entry.data[2], entry.data[3]);
	}
 
	return 0;
}

static int xc3_debugfs_fdb_raw_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, xc3_debugfs_fdb_raw_show, inode->i_private);
}

static const struct file_operations xc3_debugfs_fdb_raw_fops = {
	.open		= xc3_debugfs_fdb_raw_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int xc3_debugfs_init(struct xc3 *xc3)
{
	struct dentry *f;
	int err = -EINVAL;

	xc3->debugfs = debugfs_create_dir("xc3", NULL);
	if (!xc3->debugfs)
		goto err;

	f = debugfs_create_file("fdb_table_raw", 0444, xc3->debugfs, xc3,
				&xc3_debugfs_fdb_raw_fops);
	if (!f)
		goto err_remove;

	f = debugfs_create_file("fdb_table", 0444, xc3->debugfs, xc3,
				&xc3_debugfs_fdb_fops);
	if (!f)
		goto err_remove;

	f = debugfs_create_file("vlan_table_raw", 0444, xc3->debugfs, xc3,
				&xc3_debugfs_vlan_raw_fops);
	if (!f)
		goto err_remove;

	f = debugfs_create_file("vlan_table", 0444, xc3->debugfs, xc3,
				&xc3_debugfs_vlan_fops);
	if (!f)
		goto err_remove;

	return 0;

err_remove:
	debugfs_remove_recursive(xc3->debugfs);
err:
	return err;
}
