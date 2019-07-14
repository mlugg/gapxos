#define PAGE_TABLE_LEVELS 4

#include "paging.h"
#include <stdint.h>
#include "../pmm.h"

// Converts an address to canonical form
static uint64_t _canonical_addr(uint64_t addr) {
  uint8_t bits = 12 + 9 * PAGE_TABLE_LEVELS;
  uint8_t end = (addr >> (bits - 1)) & 1;
  if (end) addr |= ~0ULL << bits;
  else addr &= ~(~0ULL << bits);
  return addr;
}

static uint64_t *_indices_to_address(uint8_t end_level, uint16_t *indices) {
  uint64_t addr = 0;

  uint8_t wait_levels = PAGE_TABLE_LEVELS - end_level;

  for (uint8_t i = 0; i < PAGE_TABLE_LEVELS; ++i) {
    if (i < wait_levels) addr |= 511;
    else addr |= indices[i - wait_levels];
    addr <<= 9;
  }

  addr <<= 3;

  addr |= indices[end_level] << 3;

  return (void *)_canonical_addr(addr);
}

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)
#define PAGE_NO_CACHE (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY    (1 << 6)
#define PAGE_SIZE     (1 << 7)
#define PAGE_GLOBAL   (1 << 8)
#define PAGE_NO_EXEC  (1ULL << 63)

static uint16_t _pop_top_index(uint64_t *addr) {
  uint8_t addr_bits = 12 + 9 * PAGE_TABLE_LEVELS;
  uint16_t index = (*addr >> (addr_bits - 9)) & ((1 << 9) - 1);
  *addr <<= 9;
  return index;
}

struct paging_flags {
  uint8_t exec:1;
  uint8_t write:1;
  uint8_t user:1;
  uint8_t global:1;
};

// Sets the address and flags for a page table entry.
//  size: the level from the bottom at which to end the page table walk.
//    0: 4KiB page
//    1: 2MiB page
//    2: 1GiB page (support is CPU-dependent)
//  vaddr: The virtual address to change the mapping for
//  paddr: The physical address to map the page to
//  flags: The flags to set on the page
void paging_map_page(uint8_t size, uint64_t vaddr, uint64_t paddr, struct paging_flags flags) {
  uint8_t levels = PAGE_TABLE_LEVELS - size;

  uint16_t indices[levels];

  // Iterate through all parent page tables
  for (uint8_t i = 0; i < levels - 1; ++i) {
    /*
     * Calculate the index into this table, then remove from the virtual
     * address the upper 9 bits which are handled by this layer of page
     * tables
     */
    indices[i] = _pop_top_index(&vaddr);

    // Get a pointer to this entry in the tables
    uint64_t *entry = _indices_to_address(i, indices);

    // If the entry does not have the PRESENT bit set, allocate some physical memory to it
    if (!(*entry & PAGE_PRESENT)) {
      // TODO: Allocate memory
      *entry = pmm_alloc_page();

      *entry |= PAGE_PRESENT;

      // Regardless of whether we want these flags on the final page, we may as well set them here
      *entry |= PAGE_WRITE;
      *entry |= PAGE_USER;

      // We don't want the NX bit to be set, but if this is higher-half physical memory it could currently be
      // If we need the NX bit, we only need to set it on the last layer of tables
      *entry &= ~PAGE_NO_EXEC;
    }
  }

  indices[levels - 1] = _pop_top_index(&vaddr);
  uint64_t *entry = _indices_to_address(levels - 1, indices);

  // Clear lower 12 bits of physical address as they correspond to flags
  paddr &= ~0ULL << 12;

  *entry = paddr;

  *entry |= PAGE_PRESENT;
  if (!flags.exec) *entry |= PAGE_NO_EXEC;
  if (flags.write) *entry |= PAGE_WRITE;
  if (flags.user) *entry |= PAGE_USER;
  if (flags.global) *entry |= PAGE_GLOBAL;

  __asm__ volatile(
      "invlpg (%0)"
      :
      : "r" (vaddr)
      : "memory"
  );
}

static uint8_t _table_is_empty(uint64_t *table) {
  table = (uint64_t *)((uint64_t)table & ~0ULL << 12);
  for (uint16_t i = 0; i < 512; ++i) {
    if (table[i] & PAGE_PRESENT) return 0;
  }
  return 1;
}

void paging_clear_page(uint64_t vaddr) {
  uint16_t indices[PAGE_TABLE_LEVELS];

  uint64_t *entries[PAGE_TABLE_LEVELS];

  uint8_t idx;
  for (idx = 0; idx < PAGE_TABLE_LEVELS; ++idx) {
    /*
     * Calculate the index into this table, then remove from the virtual
     * address the upper 9 bits which are handled by this layer of page
     * tables
     */
    indices[idx] = _pop_top_index(&vaddr);

    // Get a pointer to this entry in the tables
    uint64_t *entry = _indices_to_address(idx, indices);

    entries[idx] = entry;

    // If the page isn't present, don't do anything
    if (!(*entry & PAGE_PRESENT)) return;

    // If the size bit is set, we've hit a huge page so we can stop walking the tables
    if (*entry & PAGE_SIZE) break;
  }

  --idx;

  // Free the page and set its entry in the lowest page table to zero
  pmm_free_page(*entries[idx] & (~0ULL << 12));
  *entries[idx] = 0;

  // While the current page table is empty and we've not hit the root of the tree, free another level up
  while (--idx && _table_is_empty(entries[idx])) {
    pmm_free_page(*entries[idx] & (~0ULL << 12));
    *entries[idx] = 0;
  }
}

void set_page(uint64_t *pml4t, uint64_t vaddr, uint64_t paddr, struct page_flags flags) {
  struct paging_flags _flags = {0};
  _flags.exec = flags.exec;
  _flags.write = flags.write;
  _flags.user = !flags.kernel;
  _flags.global = 0;
  paging_map_page(0, vaddr, paddr, _flags);
}

void free_page(uint64_t *pml4t, uint64_t vaddr) {
  paging_clear_page(vaddr);
}

void init_paging(void) {
  uint64_t *pml4t;
  __asm__ volatile(
      "movq %%cr3, %0"
      : "=r" (pml4t)
  );

  pml4t[511] = (uint64_t)pml4t | PAGE_PRESENT | PAGE_NO_EXEC;
}
