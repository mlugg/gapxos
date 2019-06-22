#pragma once

#include <stdint.h>

struct mmap_entry {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t _reserved;
} __attribute__((packed));

// Must be 1GiB-aligned
#define PHYSICAL_MAP_OFFSET (0xffffc00000000000)

struct memory_range {
  uint64_t start;
  uint64_t end;
};

void pmm_init(void *stack, struct mmap_entry *mmap, uint32_t entries, struct memory_range *unsafe, uint32_t unsafe_count);

uint64_t pmm_alloc_page();
void pmm_free_page(uint64_t addr);

extern uint32_t pmm_ap_low_page;
