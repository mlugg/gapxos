#include "system.h"
#include <stdint.h>
#include "../output/display.h"
#include "../util/basic_lib.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"
#include "acpi/acpi.h"

struct memory_manager kern_vmm;

struct memory_manager setup_kern_vmm() {
  struct memory_manager mgr = create_mgr(0xffff800001000000, 1000);

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
  kern_vmm = setup_kern_vmm();

  init_acpi(info.acpi, info.acpi_version == 2);
  if (get_table("FACP")) print("Found FADT!\n"); else print("No FADT!\n");
  if (get_table("SSDT")) print("Found SSDT!\n"); else print("No SSDT!\n");
  if (get_table("APIC")) print("Found MADT!\n"); else print("No MADT!\n");


  __asm__("hlt");
  // We should never get here
}
