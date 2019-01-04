#pragma once

#include "mem_mgmt.h"

struct avl_node *avl_find(struct avl_node *tree, uint64_t value);
struct avl_node *avl_min_size(struct avl_node *tree, uint64_t page_count);
void avl_insert(struct memory_manager *tree, uint64_t addr, uint64_t page_count);
void avl_remove(struct memory_manager *tree, uint64_t addr);
struct avl_node *avl_below(struct avl_node *tree, uint64_t addr);
