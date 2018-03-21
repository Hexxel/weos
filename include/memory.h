#ifndef __MEM_MALLOC_H
#define __MEM_MALLOC_H

#include <linux/types.h>
#include <linux/list.h>
#include <asm-generic/errno.h>

void mem_malloc_init(void *start, void *end);
ulong mem_malloc_start(void);
ulong mem_malloc_end(void);

struct memory_bank {
	struct list_head list;
	struct device_d *dev;
	unsigned long start;
	unsigned long size;
	struct resource *res;
};

extern struct list_head memory_banks;

int barebox_add_memory_bank(const char *name, resource_size_t start,
				    resource_size_t size);

#define for_each_memory_bank(mem)	list_for_each_entry(mem, &memory_banks, list)

static inline int sdram_base(unsigned long *base)
{
	struct memory_bank *b;

	b = list_first_entry_or_null(&memory_banks, struct memory_bank, list);
	if (!b)
		return -ENOENT;

	*base = b->start;
	return 0;
}

struct resource *request_sdram_region(const char *name, resource_size_t start,
		resource_size_t size);
int release_sdram_region(struct resource *res);

#endif
