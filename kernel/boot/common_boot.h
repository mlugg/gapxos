#pragma once

#include <stdint.h>

struct RSDPDescriptor {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed));

struct unsafe_range {
  void *start;
  void *end;
};

struct mmap_entry {
  void *base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t _reserved;
} __attribute__((packed));

void get_page_table_area(void **start, void **end);
void *check_memory_bounds(void *base, uint64_t len, uint64_t req_size, struct unsafe_range *unsafe, uint32_t unsafe_count);
