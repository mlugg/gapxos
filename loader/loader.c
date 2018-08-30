#include "elf64.h"
#include <stdint.h>
#include <stddef.h>

typedef uint64_t ptr64;

extern void setup_longmode();
extern void load_kernel(ptr64 kern_addr, void *mb_structure, void *krn_start, void *krn_end);
extern void setup_gdt(void *, uint32_t);

#define PAGE_PRESENT (1<<0)  // Present
#define PAGE_RW      (1<<1)  // Read and write
#define PAGE_SIZE    (1<<7)  // Points directly to a page

uint64_t *pml4t;

struct GDT {
  uint32_t base;
  uint32_t limit;
  uint32_t type;
};

void zero_memory(void *start, uint32_t len) {
  while (len--)
    *(((*(char **)&start))++) = 0;
}

void *memcpy(void *dst, const void *src, size_t length) {
  char *c_dst = dst;
  char *c_src = (char *) src;

  while (length--)
    *c_dst++ = *c_src++;

  return dst;
}

struct mmap_entry {
  ptr64 base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t _reserved;
} __attribute__((packed));

ptr64 min_elf64_addr;
struct mmap_entry *memory_map;
uint32_t mmap_entry_count;

void *find_elf64_phys_origin(uint64_t size) {
  for (int i = 0; i < mmap_entry_count; ++i) {
    if (memory_map[i].type == 1) {
      uint64_t base = memory_map[i].base_addr;
      uint64_t length = memory_map[i].length;
      if (base < min_elf64_addr) {
        int diff = min_elf64_addr - base;
        if (diff > length) continue;
        base += diff;
        length -= diff;
      }

      if (base % 0x1000) {
        uint64_t pad = base % 0x1000;
        length -= 0x1000 - pad;
        base += 0x1000 - pad;
      }

      if (length >= size)
        return (void *)(uint32_t)base;
    }
  }

  return 0;
}

void *page_free_point;

void add_page(ptr64 virt_addr, ptr64 phys_addr) {
  virt_addr &= 0xFFFFFFFFFFFF; // Removes high 16 bits
  int pml4_idx = virt_addr / 0x8000000000; // 512 GiB
  virt_addr &= 0x7FFFFFFFFF; // Removes next 9 bits
  int pdp_idx = virt_addr / 0x40000000; // 1 GiB
  virt_addr &= 0x3FFFFFFF; // Removes next 9 bits
  int pd_idx = virt_addr / 0x200000; // 2 MiB
  virt_addr &= 0x1FFFFF; // Removes 9 bits
  int p_idx = virt_addr / 0x1000; // 4 KiB

  // Indices found

  if (!(pml4t[pml4_idx] & PAGE_PRESENT)) {
    pml4t[pml4_idx] = (uint32_t) page_free_point | PAGE_PRESENT | PAGE_RW;
    page_free_point = (char *)page_free_point + 0x1000;
  }

  uint64_t *pdpt = (void *)(uint32_t)(pml4t[pml4_idx] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

  if (!(pdpt[pdp_idx] & PAGE_PRESENT)) {
    pdpt[pdp_idx] = (uint32_t) page_free_point | PAGE_PRESENT | PAGE_RW;
    page_free_point = (char *)page_free_point + 0x1000;
  }

  uint64_t *pdt = (void *)(uint32_t)(pdpt[pdp_idx] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

  if (!(pdt[pd_idx] & PAGE_PRESENT)) {
    pdt[pd_idx] = (uint32_t) page_free_point | PAGE_PRESENT | PAGE_RW;
    page_free_point = (char *)page_free_point + 0x1000;
  }

  uint64_t *pt = (void *)(uint32_t)(pdt[pd_idx] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

  pt[p_idx] = phys_addr | PAGE_PRESENT | PAGE_RW;
}

uint64_t load_elf64_module(void *mod_start, void *mod_end, void **load_start, void **load_end) {
  struct Elf64_Ehdr *ehdr = mod_start;
  
  if (ehdr->e_ident[0] != 0x7F ||
      ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L' ||
      ehdr->e_ident[3] != 'F')
    return 0; // Invalid magic number

  if (ehdr->e_ident[4] != 2)
    return 0; // Not 64-bit

  if (ehdr->e_ident[5] != 1)
    return 0; // Not little-endian

  if (ehdr->e_ident[7] != 0 ||
      ehdr->e_ident[8] != 0)
    return 0; // Not using SysV ABI

  if (ehdr->e_type != 2)
    return 0; // Not executeable

  struct Elf64_Phdr *phdr = (struct Elf64_Phdr *)(((char *)mod_start) + ehdr->e_phoff);

  ptr64 start = 0xffffffffffffffff;
  ptr64 end = 0;

  for (int i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr->p_type == 1) { // PT_LOAD
      ptr64 p_end = phdr->p_vaddr + phdr->p_memsz;
      start = start < phdr->p_vaddr ? start : phdr->p_vaddr;
      end = end > p_end ? end : p_end;
    }
    phdr = (void *)((char *)phdr + ehdr->e_phentsize);
  }

  uint64_t size = end - start;

  void *phys_location = find_elf64_phys_origin(size);

  *load_start = phys_location;
  *load_end = (char *)phys_location + size;

  // Time to read segments!

  phdr = (struct Elf64_Phdr *)(((char *)mod_start) + ehdr->e_phoff);

  for (int i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr->p_type == 1) { // PT_LOAD
      void *src = ((char *)mod_start) + phdr->p_offset;
      zero_memory(phys_location, phdr->p_memsz);
      memcpy(phys_location, src, phdr->p_filesz);
      for (int j = 0; j < phdr->p_memsz; j += 4096)
        add_page(phdr->p_vaddr + j, (ptr64)(uint32_t)phys_location + j);
      phys_location = (char *)phys_location + phdr->p_memsz;
    }
    phdr = (void *)((char *)phdr + ehdr->e_phentsize);
  }

  return ehdr->e_entry;
}


// Identity maps entire lower half of memory
void init_page_structure(void *start_mem) {
  page_free_point = start_mem;

  zero_memory(page_free_point, 1048576); // Zero the 1MiB ready for use

  pml4t = page_free_point;
  page_free_point = (char *)page_free_point + 0x1000;

  uint64_t *pdpt = page_free_point;
  page_free_point = (char *)page_free_point + 0x1000;

  uint64_t *pdt = page_free_point;
  page_free_point = (char *)page_free_point + 0x1000;
  

  // Identity map
  
  pml4t[0] = (uint32_t) pdpt | PAGE_PRESENT | PAGE_RW;
  pdpt[0] = (uint32_t) pdt | PAGE_PRESENT | PAGE_RW;
  pdt[0] = 0x000000 | PAGE_PRESENT | PAGE_RW | PAGE_SIZE; // Address is 0 MiB
  pdt[1] = 0x200000 | PAGE_PRESENT | PAGE_RW | PAGE_SIZE; // Address is 2 MiB
  pdt[2] = 0x400000 | PAGE_PRESENT | PAGE_RW | PAGE_SIZE; // Address is 4 MiB
  pdt[3] = 0x600000 | PAGE_PRESENT | PAGE_RW | PAGE_SIZE; // Address is 6 MiB
}

void encodeGdtEntry(uint8_t *target, struct GDT source)
{
  // Check the limit to make sure that it can be encoded
  if ((source.limit > 65536) && ((source.limit & 0xFFF) != 0xFFF)) {
    return; // Error
  }
  if (source.limit > 65536) {
    // Adjust granularity if required
    source.limit = source.limit >> 12;
    target[6] = 0xC0;
  } else {
    target[6] = 0x40;
  }
 
  // Encode the limit
  target[0] = source.limit & 0xFF;
  target[1] = (source.limit >> 8) & 0xFF;
  target[6] |= (source.limit >> 16) & 0xF;
 
  // Encode the base 
  target[2] = source.base & 0xFF;
  target[3] = (source.base >> 8) & 0xFF;
  target[4] = (source.base >> 16) & 0xFF;
  target[7] = (source.base >> 24) & 0xFF;
 
  // And... Type
  target[5] = source.type;
}

void lmain(void *mb_structure, void *page_structure_mem) {
  if (!mb_structure)
    __asm__("hlt");
  // TODO: Hardcode GDT
  struct GDT gdt32[3];
  gdt32[0] = (struct GDT) {.base=0, .limit=0, .type=0};
  gdt32[1] = (struct GDT) {.base=0, .limit=0x03ffffff, .type=0x9A};
  gdt32[2] = (struct GDT) {.base=0, .limit=0x03ffffff, .type=0x92};

  uint8_t gdt[24];

  encodeGdtEntry(gdt, gdt32[0]);
  encodeGdtEntry(gdt + 8, gdt32[1]);
  encodeGdtEntry(gdt + 16, gdt32[2]);

  setup_gdt(gdt, 24);

  init_page_structure(page_structure_mem);

  void *image_end = (void *)((uint32_t)page_structure_mem + 4194304);

  min_elf64_addr = (ptr64)(uint32_t)((char *)mb_structure + *(uint32_t *)(mb_structure));
  min_elf64_addr = min_elf64_addr > (uint32_t)image_end ? min_elf64_addr : (uint32_t)image_end;

  // Now find loaded kernel
  uint32_t *mb_info = mb_structure;
  mb_info += 2;

  void *mod_start, *mod_end;

  do {
    if (*mb_info == 3) {
      mod_start = *((void **)mb_info + 2);
      mod_end = *((void **)mb_info + 3);
    } else if (*mb_info == 6) {
      memory_map = (void *)((uint32_t *)mb_info + 4);
      uint32_t size = *((uint32_t *)mb_info + 1);
      uint32_t entry_size = *((uint32_t *)mb_info + 2);
      mmap_entry_count = (size - 16) / entry_size;
    }

    uint32_t size = *(mb_info + 1);
    if (size % 8)
      size += 8 - (size % 8);
    mb_info += size / 4;
  } while (*mb_info);

  min_elf64_addr = min_elf64_addr > (ptr64)(uint32_t)mod_end ? min_elf64_addr : (ptr64)(uint32_t)mod_end;

  // We have the address of the ELF64 module.
  // Whoop-de-fucking-doo.

  void *krn_start, *krn_end;

  uint64_t kernel_addr = load_elf64_module(mod_start, mod_end, &krn_start, &krn_end);

  setup_longmode(pml4t);

  load_kernel(kernel_addr, mb_structure, krn_start, krn_end);
}
