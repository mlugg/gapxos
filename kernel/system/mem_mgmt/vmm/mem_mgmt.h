#pragma once

#include <stdint.h>

struct avl_node {
  uint64_t addr;
  uint64_t page_count;
  uint8_t allocated;

  struct avl_node *left, *right;
  uint16_t height;
};


struct memory_manager {
  struct avl_node *tree;
  void *pml4t;
  uint8_t lock;
};

struct memory_manager create_mgr(uint64_t addr_start, uint64_t page_count);

void *malloc_pages(struct memory_manager *mgr, uint64_t page_count, uint8_t write, uint8_t exec, uint8_t kernel);
void *map_memory(struct memory_manager *mgr, uint64_t map_addr, uint64_t page_count, uint8_t kernel);
void free_pages(struct memory_manager *mgr, void *addr);
