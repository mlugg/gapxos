#include "system.h"
#include <stdint.h>
#include "../display.h"
#include "mem_mgmt/vmm/mem_mgmt.h"
#include "mem_mgmt/vmm/avl_tree.h"
#include "mem_mgmt/pmm.h"
#include "acpi/acpi.h"

#include "apic/madt.h"
#include "apic/processors.h"
#include "apic/init.h"

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

  print("Done stuff\n");

  void *madt = get_table("APIC");
  
  parse_madt(madt);

  disable_pic();
  enable_apic();

  init_ap_payload();
  attempt_x2apic_enable();
  start_cpu(1);

  __asm__("hlt");
  // We should never get here
}
