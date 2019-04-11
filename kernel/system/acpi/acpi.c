#include "acpi.h"

#include <stddef.h>
#include "../mem_mgmt/vmm/mem_mgmt.h"
#include "../system.h"

static uint8_t validate_checksum(struct acpi_sdt_hdr *hdr) {
  uint8_t sum = 0;

  for (int i = 0; i < hdr->length; ++i) {
    sum += ((char *) hdr)[i];
  }

  return sum == 0;
}

static uint8_t is_table(struct acpi_sdt_hdr *table, char *signature) {
  for (size_t i = 0; i < 4; ++i) {
    if (table->signature[i] != signature[i]) return 0;
  }

  return 1;
}

static void *mmap_table(uint64_t table, uint8_t kern) {
  void *mapped = map_memory(&kern_vmm, table - table % 4096, 2, kern);
  struct acpi_sdt_hdr *hdr = (void *) ((char *) mapped + table % 4096);
  if (!validate_checksum(hdr)) {
    free_pages(&kern_vmm, mapped);
    return NULL;
  }

  if (hdr->length > 4096 - table % 4096) {
    uint32_t len = hdr->length;
    len += 4096 - len % 4096;
    free_pages(&kern_vmm, mapped);
    mapped = map_memory(&kern_vmm, table - table % 4096, len / 4096 + 1, kern);
    hdr = (void *) ((char *) mapped + table % 4096);
  }

  return hdr;
}

static void unmap_table(void *table) {
  uint64_t _table = (uint64_t) table;
  _table -= _table % 4096;
  free_pages(&kern_vmm, (void *) _table);
}

static bool is_xsdt;
static struct acpi_sdt_hdr *rsdt;

void init_acpi(void *p_rsdt, bool p_is_xsdt) {
  void *mapped = mmap_table((uint64_t) p_rsdt, 1);

  if (mapped) {
    is_xsdt = p_is_xsdt;
    rsdt = mapped;
  }
}


struct acpi_sdt_hdr *get_table(char *signature) {
  if (!rsdt) return NULL;
/*
#define GET_TABLE(table_t) do { \
    size_t count = (rsdt->length - sizeof *rsdt) / sizeof (table_t); \
    table_t *tables = (table_t *) (rsdt + 1); \
    for (size_t i = 0; i < count; ++i) { \
      struct acpi_sdt_hdr *table = mmap_table(tables[i], 1); \
      if (is_table(table, signature)) return table; \
      unmap_table(table); \
    } \
  } while (0)

  if (is_xsdt) GET_TABLE(uint64_t);
  else GET_TABLE(uint32_t);

#undef GET_TABLE*/

    size_t count = (rsdt->length - sizeof *rsdt) / sizeof (uint32_t);
    uint32_t *tables = (uint32_t *) (rsdt + 1);
    for (size_t i = 0; i < count; ++i) { \
      struct acpi_sdt_hdr *table = mmap_table(tables[i], 1);
      if (is_table(table, signature)) return table;
      unmap_table(table);
    }


  return NULL;
}
