#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include "mem_mgmt.h"
#include "node_alloc.h"
#include "../pmm.h"

enum {
  PAGE_SIZE = 4096,
  USABLE_PAGE_SIZE = PAGE_SIZE - 2 * sizeof (void *), // Page size minus the pointers at the start

  // Node size in bits, + an additional bit in the header array
  NODE_BITS = sizeof (struct avl_node) * CHAR_BIT + 1,

  // Usable page size in bits divided by node size in bits
  NODES_PER_PAGE = (USABLE_PAGE_SIZE * CHAR_BIT) / NODE_BITS,

  // Number of whole 64-bit integers required for the header array
  HEADER_64S = NODES_PER_PAGE / 64,
  // Number of additional bits required for the header array
  HEADER_BITS = NODES_PER_PAGE % 64,

  // Total number of bytes required for the header array
  HEADER_BYTES = (HEADER_64S + !!HEADER_BITS) * sizeof (uint64_t),
};

// Naive memset implementation by vktec
static inline void *memset(void *p, int c, size_t n) {
  char *p2 = p;
  while (c--) *(p2++) = c;
  return p;
}

// Pointer 0 = prev, pointer 1 = next
#define PAGE_POINTERS(page) ((void **) page)
#define PAGE_HEADER(page) ((uint64_t *) ((char *) page + 2 * sizeof (void *)))
#define PAGE_NODES(page) ((struct avl_node *)((char *)page + 2 * sizeof (void *) + HEADER_BYTES))


// Initialize a page for storing AVL nodes
static inline void *init_avl_page(void *prev) {
  char *page = (char *)pmm_alloc_page();
  page += PHYSICAL_MAP_OFFSET;

  // Set the pointers at the start of the page
  PAGE_POINTERS(page)[0] = prev;
  PAGE_POINTERS(page)[1] = NULL;

  // Zero the header array
  memset(PAGE_HEADER(page), 0, HEADER_BYTES + !!HEADER_BITS);

  // Link this page to the previous page
  if (prev) PAGE_POINTERS(prev)[1] = page;

  return page;
}

// Find the offset of the first 0 bit in the provided header
static inline int find_0_bit(uint64_t *page) {
  int i, j;

  uint64_t *header = PAGE_HEADER(page);

  for (i = 0; i < HEADER_64S; ++i) {
    if (header[i] != -1) break;
  }

  int max_bit = i >= HEADER_64S ? HEADER_BITS : 64;
  for (j = 0; j < max_bit; ++j) {
    if ((header[i] >> j) & 1) continue;
    break;
  }

  if (j >= max_bit) return -1;
  return i * 64 + j;
}

static inline void set_header_bit(void *page, int idx, uint8_t val) {
  uint64_t *header = PAGE_HEADER(page);

  int byte = idx / 64;
  int bit = idx % 64;

  if (val) header[byte] |= 1 << bit;
  else header[byte] &= ~(1 << bit);
}

void *node_list_head;

// Allocates a node for use in the tree. Every page used for
// nodes has two pointers linking to the previous and next page
// and a NODES_PER_PAGE bit header which identifies which nodes
// are allocated.
struct avl_node *create_node(void) {
  // Allocate a page for the node list if we need one
  if (!node_list_head) node_list_head = init_avl_page(NULL);

  void *page = node_list_head;
  int idx;
  while ((idx = find_0_bit(page)) < 0) {
    if (PAGE_POINTERS(page)[1]) {
      page = PAGE_POINTERS(page)[1];
    } else {
      page = init_avl_page(page);
    }
  }

  set_header_bit(page, idx, 1);

  return PAGE_NODES(page) + idx;
}

static inline char *get_node_page(struct avl_node *node) {
  char *ptr = (char *) node;
  ptr -= ((uint64_t) ptr % PAGE_SIZE);
  return ptr;
}

void free_node(struct avl_node *node) {
  char *page = get_node_page(node);
  int idx = PAGE_NODES(page) - node;

  set_header_bit(page, idx, 0);

  uint64_t *header = PAGE_HEADER(page);

  for (int i = 0; i < HEADER_64S; ++i) {
    if (header[i]) return;
  }

  for (int j = 0; j < HEADER_BITS; ++j) {
    if ((header[HEADER_BYTES] >> j) & 1) return;
  }

  // We need to free this page

  if (node_list_head == page) {
    node_list_head = PAGE_POINTERS(page)[1];
    pmm_free_page((uint64_t)page);
    if (node_list_head) {
      PAGE_POINTERS(node_list_head)[0] = NULL;
    }
    return;
  }

  void *prev = PAGE_POINTERS(page)[0];
  void *next = PAGE_POINTERS(page)[1];

  PAGE_POINTERS(prev)[1] = next;
  PAGE_POINTERS(next)[0] = prev;
  pmm_free_page((uint64_t)page);
}

