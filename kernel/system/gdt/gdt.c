#include "gdt.h"
#include "../mem_mgmt/heap.h"
#include "../../util/memcpy.h"

#define GDT_ENTRY_SIZE 8

extern void _reload_gdt(void *base, uint64_t limit, uint64_t code);

struct _gdt_entry {
  uint64_t base;
  uint64_t limit;
  uint8_t type;
};

static uint64_t *_gdt_base;
static uint64_t _gdt_entries;
static uint64_t _gdt_allocated_size = 32;
static uint64_t _kern_code_seg;
static uint64_t _user_code_seg;

static uint64_t _add_entry(struct _gdt_entry e, uint8_t code_64) {
  if (e.limit > 65536 && (e.limit & 0xFFF) != 0xFFF) return 0;

  if (_gdt_allocated_size / GDT_ENTRY_SIZE == _gdt_entries++) {
    _gdt_allocated_size *= 2;
    uint64_t *new_gdt = malloc(_gdt_allocated_size);
    memcpy(new_gdt, _gdt_base, _gdt_allocated_size / 2);
    _reload_gdt(new_gdt, _gdt_entries * GDT_ENTRY_SIZE, _kern_code_seg);
    free(_gdt_base);
    _gdt_base = new_gdt;
  }

  uint8_t *target = (uint8_t *)(_gdt_base + _gdt_entries - 1);

  if (code_64) target[6] = 0x20;
  else target[6] = 0x00;

  if (e.limit > 65536) {
    e.limit = e.limit >> 12;
    target[6] |= 0x80;
  }

  target[0] = e.limit & 0xFF;
  target[1] = (e.limit >> 8) & 0xFF;
  target[6] |= (e.limit >> 16) & 0xF;

  target[2] = e.base & 0xFF;
  target[3] = (e.base >> 8) & 0xFF;
  target[4] = (e.base >> 16) & 0xFF;
  target[7] = (e.base >> 24) & 0xFF;

  target[5] = e.type;

  return (_gdt_entries - 1) * GDT_ENTRY_SIZE;
}

void gdt_init(void) {
  _gdt_base = malloc(_gdt_allocated_size);
  _add_entry((struct _gdt_entry){.base = 0, .limit = 0, .type = 0}, 0);
  _add_entry((struct _gdt_entry){.base = 0, .limit = 0, .type = 0b10010010}, 0);
  _kern_code_seg = _add_entry((struct _gdt_entry){.base = 0, .limit = 0, .type = 0b10011010}, 1);
  _add_entry((struct _gdt_entry){.base = 0, .limit = 0, .type = 0b11110010}, 0);
  _user_code_seg = _add_entry((struct _gdt_entry){.base = 0, .limit = 0, .type = 0b11111010}, 1);

  _reload_gdt(_gdt_base, _gdt_entries * GDT_ENTRY_SIZE, _kern_code_seg);
}

struct _tss {
  uint32_t _reserved_0;
  void *rsp0;
  void *rsp1;
  void *rsp2;
  uint64_t _reserved_1;
  void *ist1;
  void *ist2;
  void *ist3;
  void *ist4;
  void *ist5;
  void *ist6;
  void *ist7;
  uint64_t _reserved_2;
  uint16_t _reserved_3;
  uint16_t iopb_offset;
} __attribute__((packed));

// Each CPU needs a TSS so it can respond to interrupts in ring 0. As
// every CPU needs to be able to receive interrupts, one TSS is needed
// per CPU. This function creates a TSS for a CPU, using the given stack
// for RSP0 so it is used for interrupts. Returns the segment selector.
uint64_t gdt_add_tss(void *int_stack) {
  struct _tss *tss = malloc(sizeof *tss);
  tss->rsp0 = int_stack;
  tss->iopb_offset = sizeof *tss;
  struct _gdt_entry e = {.base = (uint64_t)tss, .limit = sizeof *tss - 1, .type = 0b10001001};
  return _add_entry(e, 0);
}

void gdt_get_info(uint16_t *limit, uint64_t *base, uint16_t *krn_code) {
  *limit = _gdt_entries * GDT_ENTRY_SIZE;
  *base = (uint64_t)_gdt_base;
  *krn_code = _kern_code_seg;
}
