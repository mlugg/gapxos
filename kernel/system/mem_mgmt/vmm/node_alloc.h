#pragma once

#include "mem_mgmt.h"

void free_node(struct avl_node *node);
struct avl_node *create_node(void);
