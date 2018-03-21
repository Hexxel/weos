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
#include <digest.h>
#include <errno.h>
#include <fcntl.h>
#include <filetype.h>
#include <fs.h>
#include <squashfs_fs.h>
#include <malloc.h>
#include <xfuncs.h>

#define BUFSIZE 0x1000

static int do_wmosum(int argc, char *argv[])
{
	struct squashfs_super_block *sb;
	struct digest *sha1;
	enum filetype type;
	u8 *sum, *expected;
	int err = 1, fd, left;

	if (argc != 2) {
		return 1;
	}

	type = file_name_detect_type(argv[1]);
	if (type != filetype_squashfs) {
		pr_err("%s is not a squashfs image\n", argv[1]);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		pr_err("could not open %s: %s\n", argv[1], errno_str());
		return 1;
	}

	sb = memmap(fd, PROT_READ);
	if (!sb) {
		pr_err("could not memmap %s: %s\n", argv[1], errno_str());
		goto out;
	}

	sha1 = digest_alloc_by_algo(HASH_ALGO_SHA1);
	if (!sha1)
		goto out;

	sum = xmalloc(digest_length(sha1));
	digest_init(sha1);

	expected = (void *)sb;
	left = le64_to_cpu(sb->bytes_used);
	while (left) {
		int sz = (left > BUFSIZE) ? BUFSIZE : left;

		digest_update(sha1, expected, sz);
		if (ctrlc()) {
			err = -EINTR;
			goto out;
		}

		left -= sz;
		expected += sz;
	}

	digest_final(sha1, sum);

	err = memcmp(sum, expected, digest_length(sha1));
	if (err)
		pr_err("checksum INVALID\n");
	else
		pr_info("checksum ok\n");

out:
	close(fd);
	return err ? 1 : 0;
}

BAREBOX_CMD_HELP_START(wmosum)
BAREBOX_CMD_HELP_TEXT("Verify WeOS squashfs images with postfixed SHA1 sum.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(wmosum)
	.cmd		= do_wmosum,
	BAREBOX_CMD_DESC("verify WeOS images")
	BAREBOX_CMD_OPTS("RAMDISK")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_wmosum_help)
BAREBOX_CMD_END
