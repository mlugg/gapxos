#include "system.h"
#include <stdint.h>
#include "../output/display.h"
#include "../util/basic_lib.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"

struct avl_node node;

struct memory_manager setup_kern_vmm() {
  struct memory_manager mgr = create_mgr(0xffff800000100000, 1000);

  uint64_t pml4t;

  __asm__ volatile(
    "movq %%cr3, %0"
    : "=r" (pml4t)
  );

  mgr.pml4t = (void *) pml4t;
  return mgr;
}

// At this point, all boot info has been dealt with. Physical memory management is set up.
void system_main(struct system_info info) {
  print("Initializing kernel virtual memory...\n");
  //struct memory_manager kern_vmm = setup_kern_vmm();

  __asm__("hlt");
  // We should never get here
}
