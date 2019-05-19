#include <stdint.h>
#include "heap.h"
#include "vmm/mem_mgmt.h"
#include "../system.h"

#define MIN_ALLOC_PAGES 2

struct heap_blkhdr {
  struct heap_blkhdr *next;

  struct heap_pghdr *pghdr;
  uint64_t length;
};

struct heap_pghdr {
  struct heap_pghdr *next;

  struct heap_blkhdr *free;
  struct heap_blkhdr *allocated;
};

static struct heap_pghdr *heap_root;

static struct heap_blkhdr *find_free(size_t size) {
  struct heap_pghdr **page = &heap_root;
  while (*page) {
    struct heap_blkhdr *block = (*page)->free;
    while (block) {
      if (block->length > size) return block;
      block = block->next;
    }

    page = &(*page)->next;
  }

  size_t pages = size / 4096;
  if (size % 4096) ++pages;
  if (pages < MIN_ALLOC_PAGES) pages = MIN_ALLOC_PAGES;

  *page = malloc_pages(&kern_vmm, pages, 1, 0, 1);

  struct heap_blkhdr *block = (struct heap_blkhdr *)(*page + 1);
  block->next = NULL;
  block->length = 4096 - sizeof **page - sizeof *block;
  block->pghdr = *page;

  (*page)->next = NULL;
  (*page)->free = block;
  (*page)->allocated = NULL;

  return block;
}

void *malloc(size_t size) {
  struct heap_blkhdr *free = find_free(size);

  if (free->length > size + sizeof (struct heap_blkhdr)) {
    free->length -= size + sizeof (struct heap_blkhdr);

    struct heap_blkhdr *allocated = (struct heap_blkhdr *)((uint8_t *)(free + 1) + free->length);
    allocated->next = NULL;
    allocated->pghdr = free->pghdr;
    allocated->length = size;

    struct heap_blkhdr **tail = &free->pghdr->allocated;
    while (*tail) tail = &(*tail)->next;
    *tail = allocated;

    return allocated + 1;
  } else {
    struct heap_blkhdr **parent = &free->pghdr->free;
    while (*parent != free) parent = &(*parent)->next;

    *parent = free->next;
    free->next = NULL;

    parent = &free->pghdr->allocated;
    while (*parent) parent = &(*parent)->next;
    *parent = free;

    return free + 1;
  }
}

void free(void *addr) {
  struct heap_blkhdr *blk = (struct heap_blkhdr *)addr - 1;
  struct heap_pghdr *pg = blk->pghdr;

  struct heap_blkhdr **parent = &pg->allocated;
  while (*parent != blk) parent = &(*parent)->next;
  *parent = blk->next;

  struct heap_blkhdr **val = &pg->free;
  for (;;) {
    if (!*val) {
      *val = blk;
      break;
    }

    if ((struct heap_blkhdr *)((uint8_t *)(*val + 1) + (*val)->length) == blk) {
      (*val)->length += blk->length + sizeof *blk;
      break;
    }

    if ((struct heap_blkhdr *)((uint8_t *)(blk + 1) + blk->length) == *val) {
      *parent = blk;
      blk->length += (*val)->length + sizeof **val;
      break;
    }

    parent = val;
    val = &(*val)->next;
  }

  if (!pg->allocated) free_pages(&kern_vmm, pg);
}
