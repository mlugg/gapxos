#include "system.h"
#include <stdint.h>
#include "../display.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"
#include "acpi/acpi.h"

#include "apic/apic.h"
#include "apic/cpu.h"

#include "mem_mgmt/vmm/paging.h"

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
  init_paging();
  kern_vmm = setup_kern_vmm();

  acpi_init(info.acpi, info.acpi_version == 2);

  print("Done stuff\n");

  void *madt = acpi_get_table("APIC");
  
  apic_parse_madt(madt);

  apic_enable();

  cpu_init_ap_payload();
  apic_enable_x2apic();
  cpu_start(1);

  __asm__("hlt");
  // We should never get here
}
