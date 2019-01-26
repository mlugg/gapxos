#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "avl_tree.h"
#include "mem_mgmt.h"
#include "../pmm.h"

struct memory_manager create_mgr(uint64_t addr_start, uint64_t page_count) {
  struct memory_manager mgr = {0};
  avl_insert(&mgr, addr_start, page_count);
  return mgr;
}

static void lock_mgr(struct memory_manager *mgr) {
  uint8_t tmp;
  do {
    __asm__ volatile(
      "movb $1, %1          \n\t"
      "lock xchgb %0, %1"
      : "+m" (mgr->lock), "+r" (tmp)
    );
  } while (tmp);
}

static void unlock_mgr(struct memory_manager *mgr) {
  __asm__ volatile(
    "movb $0, %0"
    : "=m" (mgr->lock)
  );
}

// Expects valid and locked mgr, valid page_count
static void *alloc_virt(struct memory_manager *mgr, uint64_t page_count) {
  struct avl_node *n = avl_min_size(mgr->tree, page_count);

  if (!n) return NULL;

  uint64_t addr = n->addr;
  uint64_t free_page_count = n->page_count;

  uint64_t pages_diff = free_page_count - page_count;

  if (pages_diff) {
    uint64_t post_addr = n->addr + page_count*4096;
    avl_insert(mgr, post_addr, pages_diff);
  }

  n->page_count = page_count;
  n->allocated = 1;

  return (void *) addr;
}

void *malloc_pages(struct memory_manager *mgr, uint64_t page_count, uint8_t write, uint8_t exec, uint8_t kernel) {
  if (!mgr || !page_count) return NULL;
  lock_mgr(mgr);

  void *addr = alloc_virt(mgr, page_count);

  struct page_flags flags;
  flags.write = !!write;
  flags.exec = !!exec;
  flags.kernel = !!kernel;
  flags.usable = 1;
  flags.present = 1;

  for (uint64_t i = 0; i < page_count; ++i)
    set_page(mgr->pml4t, ((uint64_t) addr) + i*0x1000, (uint64_t) alloc_phys_page(), flags);

  unlock_mgr(mgr);

  return addr;
}

void *map_memory(struct memory_manager *mgr, uint64_t map_addr, uint64_t page_count, uint8_t kernel) {
  if (!mgr || !page_count || map_addr % 0x1000) return NULL;
  lock_mgr(mgr);

  void *addr = alloc_virt(mgr, page_count);

  struct page_flags flags;
  flags.write = 0;
  flags.exec = 0;
  flags.kernel = !!kernel;
  flags.usable = 0;
  flags.present = 1;

  for (uint64_t i = 0; i < page_count; ++i)
    set_page(mgr->pml4t, ((uint64_t) addr) + i*0x1000, map_addr + i*0x1000, flags);

  unlock_mgr(mgr);

  return addr;
}

void free_pages(struct memory_manager *mgr, uint64_t addr) {
  if (!mgr) return;
  lock_mgr(mgr);

  struct avl_node *n = avl_find(mgr->tree, addr);

  if (!n || !n->allocated) {
    unlock_mgr(mgr);
    return;
  }

  n->allocated = 0;

  for (uint64_t i = 0; i < n->page_count; ++i)
    free_page(mgr->pml4t, addr + i*0x1000);

  struct avl_node *prev = avl_below(mgr->tree, addr);
  if (prev && !prev->allocated) {
    uint64_t pages_old = n->page_count;
    avl_remove(mgr, addr);
    n = prev;
    n->page_count += pages_old;
  }

  struct avl_node *after = avl_find(mgr->tree, n->addr + n->page_count * 0x1000);
  if (after && !after->allocated) {
    n->page_count += after->page_count;
    avl_remove(mgr, after->addr);
  }

  unlock_mgr(mgr);
}
