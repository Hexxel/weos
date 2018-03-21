/*
 * input.c - Allow/deny console input
 *
 * Copyright (c) 2014 Tobias Waldekranz
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
#include <command.h>

static int do_input(int argc, char *argv[])
{
	if (argc > 2)
		return -EINVAL;

	if (argc == 1) {
		/* yes, puts in barebox omits the newline */
		puts(console_is_input_allow()? "on\n" : "off\n");
		return 0;
	}

	if (!strcmp(argv[1], "on"))
		console_allow_input(true);
	else if (!strcmp(argv[1], "off"))
		console_allow_input(false);
	else
		return -EINVAL;

	return 0;
}

BAREBOX_CMD_START(input)
	.cmd		= do_input,
BAREBOX_CMD_DESC("Allow/deny console input.")
BAREBOX_CMD_OPTS("[on|off]")
BAREBOX_CMD_END
