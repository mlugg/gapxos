#pragma once

#include <stdint.h>

struct system_info {
  void *acpi;
  uint32_t acpi_version;
};

void system_main(struct system_info info);

extern struct memory_manager kern_vmm;
