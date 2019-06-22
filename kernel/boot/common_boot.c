#include "common_boot.h"
#include "../system/mem_mgmt/pmm.h"

#define PAGE_SIZE (1<<7)

// Gets the area occupied by page tables
void get_page_table_area(uint64_t *start, uint64_t *end) {
  uint64_t *pml4t;
  __asm__ volatile(
      "movq %%cr3, %0"
      : "=r" (pml4t)
  );

  *start = *end = (uint64_t)pml4t;

  for (int i = 0; i < 512; ++i) {
    uint64_t *pdpt = (void *)(pml4t[i] & 0x7FFFFFFFFFFFF000); // Remove lower 12 bits
    if ((uint64_t) pdpt & 0xf00000000000000)
      pdpt = (void *) ((uint64_t) pdpt | 0x8000000000000000);

    if (!pdpt)
      continue;

    if (pml4t[i] & PAGE_SIZE)
      continue;

    if ((uint64_t)pdpt < *start)
      *start = (uint64_t)pdpt;
    if ((uint64_t)pdpt > *end)
      *end = (uint64_t)pdpt;


    for (int j = 0; j < 512; ++j) {
      uint64_t *pdt = (void *)(pdpt[j] & 0x7FFFFFFFFFFFF000); // Remove lower 12 bits
      if ((uint64_t) pdt & 0xf00000000000000)
        pdt = (void *) ((uint64_t) pdt | 0x8000000000000000);

      if (!pdt)
        continue;

      if (pdpt[j] & PAGE_SIZE)
        continue;

      if ((uint64_t)pdt < *start)
        *start = (uint64_t)pdt;
      if ((uint64_t)pdt > *end)
        *end = (uint64_t)pdt;

      for (int k = 0; k < 512; ++k) {
        uint64_t *pt = (void *)(pdt[k] & 0x7FFFFFFFFFFFF000); // Remove lower 12 bits
        if ((uint64_t) pt & 0xf00000000000000)
          pt = (void *) ((uint64_t) pt | 0x8000000000000000);

        if (!pt)
          continue;

        if (pdt[k] & PAGE_SIZE)
          continue;

        if ((uint64_t)pt < *start)
          *start = (uint64_t)pt;
        if ((uint64_t)pt > *end)
          *end = (uint64_t)pt;

      }
    }
  }

  *end = *end + 0x1000;
}

void *check_memory_bounds(void *base, uint64_t len, uint64_t req_size, struct memory_range *unsafe, uint32_t unsafe_count) {
  char *check_end = (char *)base + len;
  char *ptr = base;
  for (uint64_t count = 0; ptr < check_end; ++ptr, ++count) {
    if (count >= req_size)
      return ptr - count;

    for (int i = 0; i < unsafe_count; ++i) {
      if ((uint64_t)ptr > (uint64_t)unsafe[i].start && (uint64_t)ptr < (uint64_t)unsafe[i].end) {
        ptr = (void *)unsafe[i].end;
        count = 0;
        break;
      }
    }
  }
  return 0;
}

uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries) {
  uint64_t size = 0;
  for (int i = 0; i < entries; ++i)
    if (mmap[i].type == 1)
      size += mmap[i].length / 4096 * sizeof(void *);
  return size;
}
