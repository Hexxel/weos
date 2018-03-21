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
#include <malloc.h>
#include <xfuncs.h>

#define BUFSIZE 0x100
struct __attribute__ ((__packed__)) bbox_header {
	uint32_t cookie;
	char platform[8];
};

static int check_barebox (uint8_t *image)
{
	struct bbox_header *bbox;
	char buf[20];
	int offset = 0;

	strncpy(buf, barebox_get_hostname(), 20);
	if (strncasecmp(buf, "basis" , 7) == 0)
		offset = 0x40;
	else if (strncasecmp(buf, "dagger" , 7) == 0)
		offset = 0x8;
	bbox = (struct bbox_header *)(image + offset);
	
	pr_info("   File device   : '%7s'\n", bbox->platform);
	pr_info("   System device : '%7s'\n", buf);
	if ((bbox->platform[0] == 0) || (strncasecmp(bbox->platform, buf, 7) != 0))
	{
		pr_info("  Error: Wrong file type!\n");
		return -1;
	}
	pr_info("   ID: OK\n");
	return 0;
}

static int do_wmover(int argc, char *argv[])
{
	int err, n, sz;
	int sfd;
	u8 src[BUFSIZE];

	err = -1;
	if (argc != 2) {
		return 1;
	}

	sfd = open(argv[1], O_RDONLY);
	if (sfd < 0) {
		pr_err("could not open %s: %s\n", argv[1], errno_str());
		return 1;
	}

	sz = BUFSIZE-1;
	n = read(sfd, src, sz);
	if (n != sz)
		goto out;

	err = check_barebox (src);
out:
	close(sfd);
	return err ? 1 : 0;
}

BAREBOX_CMD_HELP_START(wmover)
BAREBOX_CMD_HELP_TEXT("Verify meta data.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(wmover)
	.cmd		= do_wmover,
	BAREBOX_CMD_DESC("verify Bareboxs metadata")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_wmover_help)
BAREBOX_CMD_END
