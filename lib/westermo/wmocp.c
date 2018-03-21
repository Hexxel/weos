/*
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
#include <command.h>
#include <crypto/sha.h>
#include <errno.h>
#include <fcntl.h>
#include <filetype.h>
#include <fs.h>
#include <squashfs_fs.h>
#include <malloc.h>
#include <xfuncs.h>

#define BUFSIZE 0x1000

static int do_wmocp(int argc, char *argv[])
{
	struct squashfs_super_block *sb;
	enum filetype type;
	int err, n, sz, len;
	int sfd, dfd;
	u8 *dst;

	if (argc != 3) {
		return 1;
	}

	type = file_name_detect_type(argv[1]);
	if (type != filetype_squashfs) {
		pr_err("%s is not a squashfs image\n", argv[1]);
		return 1;
	}

	sfd = open(argv[1], O_RDONLY);
	if (sfd < 0) {
		pr_err("could not open %s: %s\n", argv[1], errno_str());
		return 1;
	}

	dfd = open(argv[2], O_WRONLY);
	if (dfd < 0) {
		close(sfd);

		pr_err("could not open %s: %s\n", argv[2], errno_str());
		return 1;
	}

	dst = memmap(dfd, PROT_WRITE);
	if (!dst) {
		close(dfd);
		close(sfd);

		pr_err("could not memmap %s: %s\n", argv[2], errno_str());
		return 1;
	}

	err = -EIO;

	sz = BUFSIZE;

	n = read(sfd, dst, sz);
	if (n != sz)
		goto out;

	sb = (void *)dst;
	len = le64_to_cpu(sb->bytes_used) + SHA1_DIGEST_SIZE;

	dst += sz;
	len -= sz;
	while (len > 0) {
		sz = (len > BUFSIZE) ? BUFSIZE : len;

		n = read(sfd, dst, sz);
		if (n != sz)
			goto out;

		dst += sz;
		len -= sz;

		if (ctrlc()) {
			err = -EINTR;
			goto out;
		}
	}

	err = 0;
out:
	if (err == -EIO)
		pr_err("could not copy %d bytes, only got %d\n", sz, n);

	close(dfd);
	close(sfd);
	return err ? 1 : 0;
}

BAREBOX_CMD_HELP_START(wmocp)
BAREBOX_CMD_HELP_TEXT("Copy WeOS squashfs images with postfixed SHA1 sum.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(wmocp)
	.cmd		= do_wmocp,
	BAREBOX_CMD_DESC("copy WeOS images")
	BAREBOX_CMD_OPTS("FILE RAMDISK")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_wmocp_help)
BAREBOX_CMD_END
