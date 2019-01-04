#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

struct system_info {
  void *acpi;
  uint32_t acpi_version;
};

void system_main(struct system_info info);

#endif
