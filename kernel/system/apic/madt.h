#pragma once

#include <stdint.h>
#include "../acpi/acpi.h"

struct madt {
  struct acpi_sdt_hdr hdr;
  uint32_t lapic_addr;
  uint32_t flags;
} __attribute__((packed));

void parse_madt(struct madt *madt);
