#include "system.h"
#include <stdint.h>
#include "../output/display.h"
#include "../util/basic_lib.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"

#define PAGE_PRESENT (1<<0)
#define PAGE_ADDR_MASK (0xfffffffffffff000)

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

void test_vmm(struct memory_manager *mgr) {
  char *str_mem = alloc_phys_page();
  str_mem += PHYSICAL_MAP_OFFSET;
  
  char *test1 = malloc_pages(mgr, 8, 1, 0, 1);
  print("Allocated 8 pages (32 KiB) of memory\n");
  print("Addressed at ");
  print(itoa((uint64_t) test1, str_mem, 16));
  print("\n");

  char *test2 = malloc_pages(mgr, 8, 1, 0, 1);
  print("Allocated 8 pages (32 KiB) of memory\n");
  print("Addressed at ");
  print(itoa((uint64_t) test2, str_mem, 16));
  print("\n");

  char *test3 = malloc_pages(mgr, 8, 1, 0, 1);
  print("Allocated 8 pages (32 KiB) of memory\n");
  print("Addressed at ");
  print(itoa((uint64_t) test3, str_mem, 16));
  print("\n");

  free_pages(mgr, (uint64_t) test2);
  print("Freed 8 pages (32 KiB) of memory\n");
  print("Addressed at ");
  print(itoa((uint64_t) test2, str_mem, 16));
  print("\n");


  test2 = malloc_pages(mgr, 8, 1, 0, 1);
  print("Allocated 8 pages (32 KiB) of memory\n");
  print("Addressed at ");
  print(itoa((uint64_t) test2, str_mem, 16));
  print("\n");
}

// At this point, all boot info has been dealt with. Physical memory management is set up.
void system_main(struct system_info info) {
  print("Performing VMM test...\n");
  struct memory_manager kern_vmm = setup_kern_vmm();
  test_vmm(&kern_vmm);

  __asm__("hlt");
  // We should never get here
}
