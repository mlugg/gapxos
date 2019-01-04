#pragma once

#include <stdint.h>

struct page_flags {
  unsigned write:1;    // Should user processes be able to write to this page?
  unsigned exec:1;     // Should this page be executeable?
  unsigned kernel:1;   // Should this page be supervisor-level?
  unsigned usable:1;   // Should this page be made available to the system when freed?
  unsigned present:1;  // Is the page present?
};

void set_page(uint64_t *pml4t, uint64_t vaddr, uint64_t paddr, struct page_flags flags);

void free_page(uint64_t *pml4t, uint64_t vaddr);