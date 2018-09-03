/*
 * MIT License
 *
 * Copyright (c) 2018 The gapxos Authors

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stddef.h>
#include "pmm.h"

struct physical_other_range {
  void *start;
  uint64_t size;
  enum {
    RESERVED,
    DEFECTIVE,
    HIBER_SAVE
  } type;
};

struct {
  void **free_pages; // Stack
  uint64_t free_index;
  struct physical_other_range ranges[32];
  uint16_t ranges_size;
} pmm;

// Length of stack should be available RAM amount (kb) / 4096

uint64_t get_stack_size(struct mmap_entry *mmap, uint32_t entries) {
  uint64_t size = 0;
  for (int i = 0; i < entries; ++i)
    if (mmap[i].type == 1)
      size += mmap[i].length / 4096 * sizeof(void *);   
  return size;
}

// TODO: make this less of a fuckin mess
// - What, this function?
// No, this whole project!
void init_pmm(void *stack, struct mmap_entry *mmap, uint32_t entries, struct unsafe_range *unsafe, uint32_t unsafe_count) {
  pmm.free_pages = stack;

  // Now, add all the entries
  for (int i = 0; i < entries; ++i) {
    if (mmap[i].type == 1) {
      uint64_t entry_pages = mmap[i].length / 4096;
      char *base = mmap[i].base_addr;
      if ((uint64_t)base % 0x1000)
        base += (uint64_t)base % 0x1000;

      for (int j = 0; j < entry_pages; ++j) {
        base += 4096;
        uint32_t is_free = 1;
        for (int k = 0; k < unsafe_count; ++k) {
          if ((uint64_t)base >= (uint64_t)unsafe[k].start && (uint64_t)base < (uint64_t)unsafe[k].end) {
            is_free = 0;
            break;
          }
        }

        if (is_free)
          pmm.free_pages[pmm.free_index++] = base;
      }
    } else {
      struct physical_other_range range = {0};
      range.start = mmap[i].base_addr;
      range.size = mmap[i].length;
      range.type =
        mmap[i].type == 3 ? RESERVED : // ACPI reclaimable
        mmap[i].type == 4 ? HIBER_SAVE :
        mmap[i].type == 5 ? DEFECTIVE :
        RESERVED;
      pmm.ranges[pmm.ranges_size++] = range;
    }
  }
}

void *alloc_page() {
  if (pmm.free_index)
    return pmm.free_pages[--pmm.free_index];

  return NULL;
}

void free_page(void *addr) {
  pmm.free_pages[pmm.free_index++] = addr;
}
