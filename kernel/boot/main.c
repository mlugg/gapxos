#include <stdint.h>
#include "../output/display.h"
#include "../system/mem_mgmt/pmm.h"
#include "common_boot.h"
#include "../system/system.h"

extern void load_initial_gdt(void);

struct mb_info {
  struct {
    uint32_t count;
    struct mmap_entry *map;
  } mmap;

  struct {
    uint32_t version;
    struct RSDPDescriptor *rsdp;
  } acpi;

  struct {
    uint16_t *ptr;
    uint32_t width;
    uint32_t height;
  } text;
};


static void interpret_tag(uint32_t type, uint32_t size, void *data, struct mb_info *info) {
  uint32_t *ptr = data;
  switch (type) {
    case 6:
      // Memory map
      info->mmap.count = size / *ptr;
      info->mmap.map = (void *)(ptr + 2);
      break;
    case 8:
      // Framebuffer
      ptr += 3;
      char *type_ptr = (void *)(ptr + 2);
      if (*++type_ptr == 2) {
        uint16_t **txt_ptr = data;
        info->text.ptr = *txt_ptr;
        info->text.width = *ptr++;
        info->text.height = *ptr;
      }
      break;
    case 14:
      // ACPI 1.0 RSDP
      if (info->acpi.version < 1) {
        info->acpi.rsdp = data;
        info->acpi.version = 1;
      }
      break;
    case 15:
      // ACPI 2.0 RSDP
      if (info->acpi.version < 2) {
        info->acpi.rsdp = data;
        info->acpi.version = 2;
      }
      break;
  }
}

static void interpret_multiboot(uint32_t *mb, struct mb_info *info) {
  mb += 2;

  while (*mb) {
    interpret_tag(*mb, *(mb + 1), (void *)(mb + 2), info);

    uint32_t size = *(mb + 1);
    if (size % 8)
      size += 8 - (size % 8);

    size /= 4;
    mb += size;
  }
}

void kernel_main(void *mb_structure, void *krn_start, void *krn_end, void *stack, uint64_t stack_size) {
  load_initial_gdt(); // The currently loaded GDT will probably be overwritten later; load another, very similar one.

  struct mb_info info = {0};

  interpret_multiboot(mb_structure, &info);

  init_display(info.text.ptr, info.text.width, info.text.height);

  set_color(0xF, 0x1);

  if (!info.mmap.count) {
    print("FATAL: Memory map not provided by bootloader. This is a bootloader bug!\n");
    __asm__ volatile("hlt");
  }

  if (!info.acpi.version) {
    print("FATAL: ACPI not present on system\n");
    __asm__ volatile("hlt");
  }

  if (info.acpi.rsdp->Signature[0] != 'R' ||
      info.acpi.rsdp->Signature[1] != 'S' ||
      info.acpi.rsdp->Signature[2] != 'D' ||
      info.acpi.rsdp->Signature[3] != ' ' ||
      info.acpi.rsdp->Signature[4] != 'P' ||
      info.acpi.rsdp->Signature[5] != 'T' ||
      info.acpi.rsdp->Signature[6] != 'R' ||
      info.acpi.rsdp->Signature[7] != ' ') {
    print("FATAL: Incorrect ACPI signature\n");
    __asm__ volatile("hlt");
  }

  uint32_t acpi_checksum = 0;
  uint8_t *acpi_check_ptr = (void *)info.acpi.rsdp;
  for (int i = 0; i < 20; ++i)
    acpi_checksum += acpi_check_ptr[i];

  if ((uint8_t)acpi_checksum != 0) {
    print("FATAL: Incorrect ACPI checksum\n");
    __asm__ volatile("hlt");
  }

  void *acpi_rsdt = (void *)(uint64_t)info.acpi.rsdp->RsdtAddress;

  // Now we need to init the physical memory manager by giving it some memory

  uint64_t required = get_stack_size(info.mmap.map, info.mmap.count);

  void *page_start, *page_end;
  get_page_table_area(&page_start, &page_end);

  struct unsafe_range unsafe[5];
  unsafe[0] = (struct unsafe_range) {.start = krn_start, .end = krn_end};
  unsafe[1] = (struct unsafe_range) {.start = page_start, .end = page_end};
  unsafe[2] = (struct unsafe_range) {.start = info.mmap.map, .end = info.mmap.map + info.mmap.count};
  unsafe[3] = (struct unsafe_range) {.start = stack, .end = (char *)stack + stack_size};

  void *safe = 0;

  for (int i = 0; i < info.mmap.count; ++i) {
    if (info.mmap.map[i].type == 1) {
      void *base = info.mmap.map[i].base_addr;
      uint64_t size = info.mmap.map[i].length;

      safe = check_memory_bounds(base, size, required, unsafe, 4);
      if (safe)
        break;
    }
  }

  if (!safe) {
    print("Not enough RAM!\n");
    __asm__ volatile("hlt");
  }

  // safe now contains the address of `required` bytes of contiguous physical memory

  unsafe[4] = (struct unsafe_range) {.start = safe, .end = (char *)safe + required};

  init_pmm(safe, info.mmap.map, info.mmap.count, unsafe, 5);

  struct system_info sys_info = {0};
  sys_info.acpi = acpi_rsdt;
  sys_info.acpi_version = info.acpi.version;

  system_main(sys_info);

  // We should never get here
  __asm__ volatile("hlt");
}
