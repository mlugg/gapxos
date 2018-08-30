#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../boot/common_boot.h"

uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries);
void init_pmm(void *stack, struct mmap_entry *mmap, uint32_t entries, struct unsafe_range *unsafe, uint32_t unsafe_count);

void *alloc_page();
void free_page(void *addr);

#endif
