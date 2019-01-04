#include <stdint.h>
#include <stddef.h>
#include "avl_tree.h"
#include "mem_mgmt.h"

uint16_t height(struct avl_node *node) {
  if (!node)
    return 0;
  
  uint16_t lh = node->left ? node->left->height + 1 : 0;
  uint16_t rh = node->right ? node->right->height + 1 : 0;

  return lh > rh ? lh : rh;
}

int8_t balance(struct avl_node *node) {
  if (!node)
    return 0;
  
  int32_t lh = node->left ? node->left->height + 1 : 0;
  int32_t rh = node->right ? node->right->height + 1 : 0;

  return rh - lh;
}

struct avl_node *rotate_left(struct avl_node *x) {
  struct avl_node *z = x->right;
  x->right = z->left;
  z->left = x;
  x->height = height(x);
  z->height = height(z);
  return z;
}

struct avl_node *rotate_right(struct avl_node *x) {
  struct avl_node *z = x->left;
  x->left = z->right;
  z->right = x;
  x->height = height(x);
  z->height = height(z);
  return z;
}

//TODO: good things
uint64_t free_nodez_idx;
struct avl_node free_nodez[256];

struct avl_node *create_node() {
  return free_nodez + free_nodez_idx++;
}

void free_node(struct avl_node *node) {
  free_nodez_idx--;
}

struct avl_node *_insert_node(struct avl_node *node, uint64_t addr, uint64_t page_count) {
  if (!node) {
    node = create_node();
    node->addr = addr;
    node->page_count = page_count;
  } else if (addr < node->addr) {
    node->left = _insert_node(node->left, addr, page_count);
  } else if (addr > node->addr) {
    node->right = _insert_node(node->right, addr, page_count);
  } else {
    node->addr = addr;
    node->page_count = page_count;
    return node;
  }

  int8_t b = balance(node);
  if (b == -2) {
    if (balance(node->left) == 1)
      node->left = rotate_left(node->left);
    
    node = rotate_right(node);
  } else if (b == 2) {
    if (balance(node->right) == -1)
      node->right = rotate_right(node->right);

    node = rotate_left(node);
  }

  node->height = height(node);

  return node;
}

struct avl_node *_delete_node(struct avl_node *node, uint64_t addr) {
  if (!node)
    return NULL;
  
  if (addr < node->addr) {
    node->left = _delete_node(node->left, addr);
  } else if (addr > node->addr) {
    node->right = _delete_node(node->right, addr);
  } else {
    if (!node->left && !node->right) {
      free_node(node);
      return NULL;
    } else if (node->left && !node->right) {
      struct avl_node *left = node->left;
      free_node(node);
      node = left;
    } else if (!node->left && node->right) {
      struct avl_node *right = node->right;
      free_node(node);
      node = right;
    } else {
      struct avl_node *ptr = node->right, *parent;
      while (ptr->left) {
        parent = ptr;
        ptr = ptr->left;
      }
      node->addr = ptr->addr;
      node->page_count = ptr->page_count;
      if (ptr->right && !parent)
        node->right = ptr->right;
      else if (ptr->right)
        parent->left = ptr->right;
      else if (parent)
        parent->left = NULL;

      free_node(ptr);
    }
  }

  int8_t b = balance(node);
  if (b == -2) {
    if (balance(node->left) == 1)
      node->left = rotate_left(node->left);
    
    node = rotate_right(node);
  } else if (b == 2) {
    if (balance(node->right) == -1)
      node->right = rotate_right(node->right);
    
    node = rotate_left(node);
  }

  node->height = height(node);
  return node;
}

// Insert node
void avl_insert(struct memory_manager *tree, uint64_t addr, uint64_t page_count) {
  tree->tree = *_insert_node(&tree->tree, addr, page_count);
}

// Remove a node by address
void avl_remove(struct memory_manager *tree, uint64_t addr) {
  tree->tree = *_delete_node(&tree->tree, addr);
}

// Iterating backwards, find a free node at least page_count pages long
struct avl_node *avl_min_size(struct avl_node *node, uint64_t page_count) {
  if (node->right) {
    struct avl_node *r = avl_min_size(node->right, page_count);
    if (r)
      return r;
  }

  if (node->page_count >= page_count)
    return node;

  if (node->left)
    return avl_min_size(node->left, page_count);
  
  return NULL;
}

// Find the node directly before addr
struct avl_node *avl_below(struct avl_node *node, uint64_t addr) {
  int diff = addr - (node->addr + node->page_count * 0x1000);
  
  if (diff == 0)
    return node;
  
  if (diff < 0) {
    if (node->left)
      return avl_below(node->left, addr);

    return NULL;
  }

  if (node->right)
    return avl_below(node->right, addr);
  
  return NULL;
}

// Find the node with the address addr
struct avl_node *avl_find(struct avl_node *node, uint64_t addr) {
  if (node->addr == addr)
    return node;
  
  if (addr < node->addr) {
    if (node->left)
      return avl_find(node->left, addr);
    
    return NULL;
  }

  if (node->right)
    return avl_find(node->right, addr);

  return NULL;
}
