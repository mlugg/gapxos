#pragma once

#include <stdint.h>
#include "../system/mem_mgmt/pmm.h"

struct RSDPDescriptor {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed));

void get_page_table_area(uint64_t *start, uint64_t *end);
void *check_memory_bounds(void *base, uint64_t len, uint64_t req_size, struct memory_range *unsafe, uint32_t unsafe_count);
uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries);
