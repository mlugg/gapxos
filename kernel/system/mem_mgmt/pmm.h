#pragma once

#include <stdint.h>
#include "../../boot/common_boot.h"

// Must be 1GiB-aligned
#define PHYSICAL_MAP_OFFSET (0xffffc00000000000)

uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries);
void init_pmm(void *stack, struct mmap_entry *mmap, uint32_t entries, struct unsafe_range *unsafe, uint32_t unsafe_count);

void *alloc_phys_page();
void free_phys_page(void *addr);
