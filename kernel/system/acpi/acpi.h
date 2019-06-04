#pragma once

#include <stdint.h>
#include <stdbool.h>

struct acpi_sdt_hdr {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oemid[6];
  char oemtableid[8];
  uint32_t oemrevision;
  uint32_t creatorid;
  uint32_t creatorrevision;
};

struct acpi_gas {
  uint8_t address_space;
  uint8_t bit_width;
  uint8_t bit_offset;
  uint8_t access_size;
  uint64_t addr;
};

void acpi_init(void *p_rsdt, bool p_is_xsdt);
struct acpi_sdt_hdr *acpi_get_table(char *signature);
