#include "system.h"
#include <stdint.h>
#include "../output/display.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"

#define PAGE_PRESENT (1<<0)
#define PAGE_ADDR_MASK (0xfffffffffffff000)

struct avl_node node;

struct memory_manager setup_kern_vmm() {
  struct memory_manager mgr = {0};

  uint64_t pml4t;

  __asm__ volatile(
    "movq %%cr3, %0"
    : "=r" (pml4t)
  );

  mgr.pml4t = (void *) pml4t;
  mgr.tree.addr = 0xffff800000100000;
  mgr.tree.page_count = 1000;
  return mgr;
}

// At this point, all boot info has been dealt with. Physical memory management is set up.
void system_main(struct system_info info) {
  print("Hi!\n");
  struct memory_manager kern_vmm = setup_kern_vmm();

  print("Hi!\n");
  char *test = malloc_pages(&kern_vmm, 10, 1, 0, 1);

  print("Hi!\n");
  for (int i = 0; i < 10; ++i)
    test[i] = i;

  print("Hi!\n");
  char *test2 = malloc_pages(&kern_vmm, 10, 1, 0, 1);

  print("Hi!\n");
  for (int i = 0; i < 10; ++i)
    test2[i] = i + 64;

  print("Hi!\n");
  for (int i = 0; i < 10; ++i)
    if (test[i] != i)
      print("Soemthing broke\n");
  print("Hi!\n");

  for (int i = 0; i < 10; ++i)
    if (test2[i] != i + 64)
      print("Something broke\n");

  //__asm__("hlt");
  print("Where were you while we were getting...\n");
  print("Hi!");
  // We should never get here
}
