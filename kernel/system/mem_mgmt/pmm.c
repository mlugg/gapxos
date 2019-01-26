#include <stddef.h>
#include "pmm.h"

struct physical_other_range {
  void *start;
  uint64_t size;
  enum {
    RESERVED,
    DEFECTIVE,
    HIBER_SAVE
  } type;
};

struct {
  void **free_pages; // Stack
  uint64_t free_index;
  struct physical_other_range ranges[32];
  uint16_t ranges_size;
} pmm;

#define PAGE_PRESENT (1<<0)
#define PAGE_RW      (1<<1)
#define PAGE_SIZE    (1<<7)
#define PAGE_GLOBAL  (1<<8)

static inline void zero_page(void *page) {
  for (char *ptr = page; (uint64_t)ptr < (uint64_t)page + 4096; ++ptr)
    *ptr = 0;
}

// This is only used in init, so doesn't have to be too fast.
static void *get_low_page() {
  int lowest_idx = 0;
  for (uint64_t i = 0; i < pmm.free_index; ++i) {
    if ((uint64_t)pmm.free_pages[i] < (uint64_t)pmm.free_pages[lowest_idx])
      lowest_idx = i;
  }

  void *val = pmm.free_pages[lowest_idx];

  for (uint64_t i = lowest_idx + 1; i < pmm.free_index; ++i)
    pmm.free_pages[i-1] = pmm.free_pages[i];

  --pmm.free_index;

  return val;
}

// Goes from the state the loader leaves us in and pages all of physical memory
static void init_paging(uint64_t phys_mem_length) {
  uint64_t phys_map_offset_real = PHYSICAL_MAP_OFFSET & 0xffffffffffff; // Drops the top 12 bits
  uint64_t *pml4t;
  __asm__ volatile(
      "movq %%cr3, %0"
      : "=r" (pml4t)
  );

  for (uint64_t i = 0; i < phys_mem_length; i += 0x40000000) { // Every GiB
    // We need some memory near the start of RAM
    // This is because the loader will have identity paged it
    uint64_t *page = get_low_page();
    zero_page(page);
    // We have a free page of memory
    // We can use it to map 1GiB of RAM using 2MiB pages
    // Map the first GiB

    // On each iteration:
    // - Map the correct 2MiB
    // - Make the page present and read/write
    // - Set SIZE to make the entry an actual page
    // - Set GLOBAL as these will always be mapped
    for (int j = 0; j < 512; ++j)
      page[j] = (0x200000 * j + i)
        | PAGE_PRESENT | PAGE_RW | PAGE_SIZE | PAGE_GLOBAL;


    uint64_t offset = phys_map_offset_real + i;

    // We want to add that to the correct PDPT
    // However, it might not exist
    // If it doesn't, we need another page at the bottom of memory
    uint64_t pml4 = pml4t[offset / 0x8000000000];
    if (!(pml4 & PAGE_PRESENT)) {
      // We need to allocate another page for the PDPT
      uint64_t *ptr = get_low_page();
      zero_page(ptr);
      pml4 = pml4t[offset / 0x8000000000] = (uint64_t)ptr | PAGE_PRESENT | PAGE_RW;
    }
    uint64_t *pdpt = (void *)(pml4 & 0xFFFFFFFFFFFFF000);
    int idx = offset % 0x8000000000 / 0x40000000;
    pdpt[idx] = (uint64_t)page | PAGE_PRESENT | PAGE_RW;
  }
}

// Length of stack should be available RAM amount (kb) / 4096

uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries) {
  uint64_t size = 0;
  for (int i = 0; i < entries; ++i)
    if (mmap[i].type == 1)
      size += mmap[i].length / 4096 * sizeof(void *);   
  return size;
}

// TODO: make this less of a fuckin mess
// - What, this function?
// No, this whole project!
void init_pmm(void *stack, struct mmap_entry *mmap, uint32_t entries, struct unsafe_range *unsafe, uint32_t unsafe_count) {
  pmm.free_pages = stack;

  uint64_t mem_max = 0;

  // Now, add all the entries
  for (int i = 0; i < entries; ++i) {
    if ((uint64_t)mmap[i].base_addr + mmap[i].length > mem_max)
      mem_max = (uint64_t)mmap[i].base_addr + mmap[i].length;

    if (mmap[i].type == 1) {
      uint64_t entry_pages = mmap[i].length / 4096;
      char *base = mmap[i].base_addr;
      if ((uint64_t)base % 0x1000)
        base += (uint64_t)base % 0x1000;

      for (int j = 0; j < entry_pages; ++j) {
        base += 4096;
        uint32_t is_free = 1;
        for (int k = 0; k < unsafe_count; ++k) {
          if ((uint64_t)base >= (uint64_t)unsafe[k].start && (uint64_t)base < (uint64_t)unsafe[k].end) {
            is_free = 0;
            break;
          }
        }

        if (is_free)
          pmm.free_pages[pmm.free_index++] = base;
      }
    } else {
      struct physical_other_range range = {0};
      range.start = mmap[i].base_addr;
      range.size = mmap[i].length;
      range.type =
        mmap[i].type == 3 ? RESERVED : // ACPI reclaimable
        mmap[i].type == 4 ? HIBER_SAVE :
        mmap[i].type == 5 ? DEFECTIVE :
        RESERVED;
      pmm.ranges[pmm.ranges_size++] = range;
    }
  }

  // Page all physical memory somewhere for us
  init_paging(mem_max);
}

void *alloc_phys_page() {
  if (pmm.free_index)
    return pmm.free_pages[--pmm.free_index];

  return NULL;
}

void free_phys_page(void *addr) {
  pmm.free_pages[pmm.free_index++] = addr;
}
