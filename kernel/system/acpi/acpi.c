#include "acpi.h"

#include <stddef.h>
#include "../mem_mgmt/vmm/mem_mgmt.h"
#include "../system.h"

static uint8_t _validate_checksum(struct acpi_sdt_hdr *hdr) {
  uint8_t sum = 0;

  for (int i = 0; i < hdr->length; ++i) {
    sum += ((char *)hdr)[i];
  }

  return sum == 0;
}

static uint8_t _is_table(struct acpi_sdt_hdr *table, char *signature) {
  for (size_t i = 0; i < 4; ++i) {
    if (table->signature[i] != signature[i]) return 0;
  }

  return 1;
}

static void *_mmap_table(uint64_t table) {
  void *mapped = map_memory(&kern_vmm, table - table % 4096, 2, 1);
  struct acpi_sdt_hdr *hdr = (void *)((char *)mapped + table % 4096);
  if (!_validate_checksum(hdr)) {
    free_pages(&kern_vmm, mapped);
    return NULL;
  }

  if (hdr->length > 4096 - table % 4096) {
    uint32_t len = hdr->length;
    len += 4096 - len % 4096;
    free_pages(&kern_vmm, mapped);
    mapped = map_memory(&kern_vmm, table - table % 4096, len / 4096 + 1, 1);
    hdr = (void *)((char *)mapped + table % 4096);
  }

  return hdr;
}

static void _unmap_table(void *table) {
  uint64_t _table = (uint64_t)table;
  _table -= _table % 4096;
  free_pages(&kern_vmm, (void *)_table);
}

static bool _is_xsdt;
static struct acpi_sdt_hdr *_rsdt;

void acpi_init(void *rsdt, bool is_xsdt) {
  void *mapped = _mmap_table((uint64_t)rsdt);

  if (mapped) {
    _is_xsdt = is_xsdt;
    _rsdt = mapped;
  }
}


struct acpi_sdt_hdr *acpi_get_table(char *signature) {
  if (!_rsdt) return NULL;

#define GET_TABLE(table_t) do { \
    size_t count = (_rsdt->length - sizeof *_rsdt) / sizeof (table_t); \
    table_t *tables = (table_t *) (_rsdt + 1); \
    for (size_t i = 0; i < count; ++i) { \
      struct acpi_sdt_hdr *table = _mmap_table(tables[i]); \
      if (_is_table(table, signature)) return table; \
      _unmap_table(table); \
    } \
  } while (0)

  if (_is_xsdt) GET_TABLE(uint64_t);
  else GET_TABLE(uint32_t);

#undef GET_TABLE

  return NULL;
}
