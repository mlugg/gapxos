#include <stdint.h>
#include "paging.h"
#include "../pmm.h"

#define _PAGE_PRESENT (1<<0)
#define _PAGE_RW      (1<<1)
#define _PAGE_USER    (1<<2)
#define _PAGE_GLOBAL  (1<<8)
//#define _PAGE_NX      (1<<63)
#define _PAGE_NX      (0x8000000000000000)
#define _PAGE_CUSTOM_USABLE (1<<9)

// Addresses must have the most significant 16 bits equal the next most significant bit. Sometimes we don't want this 
#define _MASK_REMOVE_16U (0xffffffffffff)

// The lower 12 bits are all flags in page tables
#define _MASK_REMOVE_12L (0xfffffffff000)

static uint64_t *get_page_table_entry(uint64_t *pml4t, uint64_t vaddr, uint64_t flags_add) {
  uint64_t offset = PHYSICAL_MAP_OFFSET / 8;
  pml4t += offset;

  int i = vaddr / 0x8000000000;
  vaddr %= 0x8000000000;
  if (!(pml4t[i] & _PAGE_PRESENT))
    pml4t[i] = (uint64_t) alloc_phys_page() | _PAGE_PRESENT;
  pml4t[i] |= flags_add;
  uint64_t *pdpt = (void *)(pml4t[i] & _MASK_REMOVE_12L);
  pdpt += offset;
  
  i = vaddr / 0x40000000;
  vaddr %= 0x40000000;
  if (!(pdpt[i] & _PAGE_PRESENT))
    pdpt[i] = (uint64_t) alloc_phys_page() | _PAGE_PRESENT;
  pdpt[i] |= flags_add;
  uint64_t *pdt = (void *)(pdpt[i] & _MASK_REMOVE_12L);
  pdt += offset;

  i = vaddr / 0x200000;
  vaddr %= 0x200000;
  if (!(pdt[i] & _PAGE_PRESENT))
    pdt[i] = (uint64_t) alloc_phys_page() | _PAGE_PRESENT;
  pdt[i] |= flags_add;
  uint64_t *pt = (void *)(pdt[i] & _MASK_REMOVE_12L);
  pt += offset;

  i = vaddr / 0x1000;
  return pt + i;
}

// Sets the page tables for a given vaddr to a certain paddr, applying the given flags
// Expects for nothing else to be writing to the page tables to avoid race conditions
// If physical page is PRESENT and USABLE, it is first freed.
void set_page(uint64_t *pml4t, uint64_t vaddr, uint64_t paddr, struct page_flags flags) {
  uint64_t table_flags = 0;

  table_flags |= flags.present ? _PAGE_PRESENT : 0;
  table_flags |= flags.write ? _PAGE_RW : 0;
  table_flags |= flags.exec ? 0 : _PAGE_NX;
  table_flags |= flags.kernel ? _PAGE_GLOBAL : _PAGE_USER;
  table_flags |= flags.usable ? _PAGE_CUSTOM_USABLE : 0;

  vaddr &= _MASK_REMOVE_16U;
  paddr &= _MASK_REMOVE_12L & ~(_PAGE_NX); // The NX bit is in the upper place

  uint64_t *page = get_page_table_entry(pml4t, vaddr, table_flags);
  *page = paddr | table_flags;

  // TODO: Invalidate page
}

void free_page(uint64_t *pml4t, uint64_t vaddr) {
  vaddr &= _MASK_REMOVE_16U;
  uint64_t *page = get_page_table_entry(pml4t, vaddr, 0);
  
  if (*page & _PAGE_CUSTOM_USABLE) {
    uint64_t addr = *page & _MASK_REMOVE_12L;
    free_phys_page((void *) addr);
  }

  *page = 0;

  // TODO: Invalidate page
}
