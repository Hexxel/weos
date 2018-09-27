/*
 * flattened image tree (fit) library functions
 *
 * Author: Tobias Waldekranz <tobias@waldekranz.com>
 *
 * Copyright (c) 2014 Westermo Teleindustri AB
 */

#ifndef __FIT_H
#define __FIT_H

#include <boot.h>
#include <bootm.h>

#ifndef CONFIG_FIT_IMAGE
static inline int fit_looks_ok(const void *fit)
{
	return 0;
}

static inline int fit_prepare(struct image_data *data)
{
	return -1;
}
#else
int fit_looks_ok(const void *fit);
int fit_prepare(struct image_data *data);
#endif /* CONFIG_FIT_IMAGE */

#endif	/* __FIT_H */
