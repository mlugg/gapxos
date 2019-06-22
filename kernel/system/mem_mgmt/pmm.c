#include <stddef.h>
#include <stdbool.h>
#include "pmm.h"

struct _physical_other_range {
  uint64_t start;
  uint64_t size;
  enum {
    RESERVED,
    DEFECTIVE,
    HIBER_SAVE
  } type;
};

uint64_t *_free_pages; // Stack
uint64_t _free_index;
struct _physical_other_range _ranges[32];
uint16_t _ranges_size;

#define PAGE_PRESENT (1<<0)
#define PAGE_RW      (1<<1)
#define PAGE_SIZE    (1<<7)
#define PAGE_GLOBAL  (1<<8)

static inline void _zero_page(void *page) {
  for (char *ptr = page; (uint64_t)ptr < (uint64_t)page + 4096; ++ptr)
    *ptr = 0;
}

// This is only used in init, so doesn't have to be too fast.
static uint64_t _get_low_page() {
  int lowest_idx = 0;
  for (uint64_t i = 0; i < _free_index; ++i) {
    if (_free_pages[i] < _free_pages[lowest_idx])
      lowest_idx = i;
  }

  uint64_t val = _free_pages[lowest_idx];

  for (uint64_t i = lowest_idx + 1; i < _free_index; ++i)
    _free_pages[i-1] = _free_pages[i];

  --_free_index;

  return val;
}

// Goes from the state the loader leaves us in and pages all of physical memory
static void _init_paging(uint64_t phys_mem_length) {
  uint64_t phys_map_offset_real = PHYSICAL_MAP_OFFSET & 0xffffffffffff; // Drops the top 12 bits
  uint64_t *pml4t;
  __asm__ volatile(
      "movq %%cr3, %0"
      : "=r" (pml4t)
  );

  for (uint64_t i = 0; i < phys_mem_length; i += 0x40000000) { // Every GiB
    // We need some memory near the start of RAM
    // This is because the loader will have identity paged it
    uint64_t *page = (uint64_t *)_get_low_page();
    _zero_page(page);
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
      uint64_t *ptr = (uint64_t *)_get_low_page();
      _zero_page(ptr);
      pml4 = pml4t[offset / 0x8000000000] = (uint64_t)ptr | PAGE_PRESENT | PAGE_RW;
    }
    uint64_t *pdpt = (void *)(pml4 & 0xFFFFFFFFFFFFF000);
    int idx = offset % 0x8000000000 / 0x40000000;
    pdpt[idx] = (uint64_t)page | PAGE_PRESENT | PAGE_RW;
  }
}


// We need a single page of low memory (<1MiB) to be reserved.
// This is where AP init code will go. It will later be freed.
uint32_t pmm_ap_low_page;

static inline uint8_t _is_in_range(uint64_t page, struct memory_range range) {
  return page >= range.start && page < range.end;
}

void pmm_init(void *stack, struct mmap_entry *mmap, uint32_t entries, struct memory_range *unsafe, uint32_t unsafe_count) {
  _free_pages = stack;

  uint64_t max_addr = 0;
  for (size_t i = 0; i < entries; ++i) {
    if (mmap[i].base_addr + mmap[i].length > max_addr) {
      max_addr = mmap[i].base_addr + mmap[i].length;
    }

    if (mmap[i].type == 1) {
      uint64_t start = mmap[i].base_addr;
      if (start % 4096) {
        start += 4096 - (start % 4096);
      }
      uint64_t end = mmap[i].base_addr + mmap[i].length;
      end -= end % 4096;

      for (uint64_t cur = start; cur < end; cur += 4096) {
        bool is_free = true;
        for (size_t k = 0; k < unsafe_count; ++k) {
          if (_is_in_range(cur, unsafe[k])) {
            is_free = false;
            break;
          }
        }

        if (is_free) {
          if (!pmm_ap_low_page && cur < (1<<20)) pmm_ap_low_page = cur;
          else _free_pages[_free_index++] = cur;
        }
      }
    } else {
      struct _physical_other_range range = {0};
      range.start = mmap[i].base_addr;
      range.size = mmap[i].length;
      range.type =
        mmap[i].type == 3 ? RESERVED : // TODO: This is actually reclaimable (ACPI)
        mmap[i].type == 4 ? HIBER_SAVE :
        mmap[i].type == 5 ? DEFECTIVE :
        RESERVED;
      _ranges[_ranges_size++] = range;
    }
  }

  _init_paging(max_addr);
}

uint64_t pmm_alloc_page() {
  if (_free_index)
    return _free_pages[--_free_index];

  return 0;
}

void pmm_free_page(uint64_t addr) {
  _free_pages[_free_index++] = addr;
}
