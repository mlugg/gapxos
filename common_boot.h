#ifndef COMMON_BOOT_H
#define COMMON_BOOT_H

#include <stdint.h>

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

#endif
