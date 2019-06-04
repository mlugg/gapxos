#include "apic.h"
#include "../acpi/acpi.h"
#include <stddef.h>
#include "../mem_mgmt/heap.h"
#include "../mem_mgmt/pmm.h"

static void _outb(uint16_t port, uint8_t val) {
  __asm__ volatile(
      "outb %0, %1"
      :: "r"(val), "r"(port)
  );
}

struct {
  uint8_t present:1;
  uint8_t lint:1;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;
} global_nmi_lint;

struct apic_info apic_info;


// {{{ MADT parsing
struct _madt {
  struct acpi_sdt_hdr hdr;
  uint32_t lapic_addr;
  uint32_t flags;
} __attribute__((packed));

struct _local_apic_flags {
  uint32_t enabled:1;
  uint32_t online_capable:1;
  uint32_t _reserved:30;
};

struct _mps_inti_flags {
  uint16_t polarity:2;
  uint16_t trigger_mode:2;
  uint16_t _reserved:12;
};

static void _register_lapic(uint32_t proc_uid, uint32_t apic_id, struct _local_apic_flags flags, enum apic_lapic_mode mode) {
  // Check if an entry already exists in the list
  for (struct apic_lapic *x = apic_info.lapic; x; x = x->next) {
    if (x->proc_uid == proc_uid) {
      x->lapic_id = apic_id;
      x->mode = mode;
      x->enabled = flags.enabled;
      x->online_capable = flags.online_capable;
      return;
    }
  }

  // Create an entry
  struct apic_lapic *new = malloc(sizeof *new);
  new->next = apic_info.lapic;
  apic_info.lapic = new;

  new->proc_uid = proc_uid;
  new->lapic_id = apic_id;
  new->mode = mode;
  new->enabled = flags.enabled;
  new->online_capable = flags.online_capable;
  new->nmi.present = 0;
}

static void _register_ioapic(uint8_t id, uint32_t addr, uint32_t gsi_base) {
  // Create an entry
  struct apic_io_apic *new = malloc(sizeof *new);
  new->next = apic_info.io_apic;
  apic_info.io_apic = new;

  new->id = id;
  uint32_t volatile *vaddr = (uint32_t volatile *)(PHYSICAL_MAP_OFFSET + addr);
  vaddr[0] = 1;
  new->gdi_count = (vaddr[4] >> 16) & 0xff;
  new->paddr = addr;
  new->gsi_base = gsi_base;
}

static void _register_iso(uint8_t irq, uint32_t gsi, struct _mps_inti_flags flags) {
  // Create an entry
  struct apic_irq_map *new = malloc(sizeof *new);
  new->next = apic_info.irq_gsi_map;
  apic_info.irq_gsi_map = new;

  new->gsi = gsi;
  new->irq = irq;
  new->polarity = flags.polarity;
  new->trigger_mode = flags.trigger_mode;
}

static void _register_nmi_source(struct _mps_inti_flags flags, uint32_t gsi) {
  // Create an entry
  struct apic_nmi_source *new = malloc(sizeof *new);
  new->next = apic_info.nmi_source;
  apic_info.nmi_source = new;

  new->gsi = gsi;
  new->polarity = flags.polarity;
  new->trigger_mode = flags.trigger_mode;
}

static void _set_lapic_nmi_lint(uint32_t proc_uid, struct _mps_inti_flags flags, uint8_t lint_no) {
  if (proc_uid == 0xFF) {
    // Applies to all CPUs
    global_nmi_lint.present = 1;
    global_nmi_lint.lint = lint_no;
    global_nmi_lint.polarity = flags.polarity;
    global_nmi_lint.trigger_mode = flags.trigger_mode;
    return;
  }
  // Check if an entry already exists in the list
  for (struct apic_lapic *x = apic_info.lapic; x; x = x->next) {
    if (x->proc_uid == proc_uid) {
      x->nmi.present = 1;
      x->nmi.lint = lint_no;
      x->nmi.polarity = flags.polarity;
      x->nmi.trigger_mode = flags.trigger_mode;
      return;
    }
  }

  // Create an entry
  struct apic_lapic *new = malloc(sizeof *new);
  new->next = apic_info.lapic;
  apic_info.lapic = new;

  new->proc_uid = proc_uid;
  new->nmi.present = 1;
  new->nmi.lint = lint_no;
  new->nmi.polarity = flags.polarity;
  new->nmi.trigger_mode = flags.trigger_mode;
}

void apic_parse_madt(void *madt) {
  apic_info.lapic_addr = ((struct _madt *)madt)->lapic_addr;
  
  uint8_t *end = (uint8_t *)madt + ((struct _madt *)madt)->hdr.length;

  for (uint8_t *entry = (uint8_t *)madt + sizeof (struct _madt); entry < end; entry += entry[1]) {
    switch (entry[0]) {
    case 0:
      // *** Legacy local APIC structure ***
      // Describes a single local APIC for a processor operating in
      // legacy mode with a 1-byte ID.
      _register_lapic(entry[2], entry[3], ((struct _local_apic_flags *)entry)[1], MODE_LEGACY_APIC);
      break;
    case 9:
      // *** x2APIC local APIC structure ***
      // Describes a single local APIC for a processor operating in
      // x2APIC mode with a 4-byte ID.
      _register_lapic(((uint32_t *)entry)[1], ((uint32_t *)entry)[2], ((struct _local_apic_flags *)entry)[3], MODE_X2APIC);
      break;
    case 1:
      // *** I/O APIC structure ***
      // Describes a single I/O APIC. Specifies the 1-byte ID, the
      // 32-bit physical address of the APIC's registers, and the global
      // system interrupts it is responsible for.
      _register_ioapic(entry[2], (((uint32_t *)entry)[1]), ((uint32_t *)entry)[2]);
      break;
    case 2:
      // *** Interrupt source override structure ***
      // Contains a mapping between the 8259 PIC IRQs and the APIC
      // interrupts. The IRQs are assumed to be identity mapped to the
      // APIC interrupts; however, most systems will have one or more
      // exceptions to this rule, which are conveyed through this
      // structure.
      _register_iso(entry[3], ((uint32_t *)entry)[1], ((struct _mps_inti_flags *)entry)[4]);
      break;
    case 3:
      // *** NMI source structure ***
      // Specifies a Global System Interrupt which should be set as
      // non-maskable. It is equivalent to a classical NMI, indicating a
      // hardware fault that the OS needs to deal with.
      _register_nmi_source(((struct _mps_inti_flags *)entry)[1], ((uint32_t *)entry)[1]);
      break;
    case 4:
      // *** Legacy local APIC NMI structure ***
      // Describes the local APIC interrupt number that the NMI is
      // connected to on a legacy LAPIC. This should be set as an NMI so
      // the system can deal with hardware faults.
      _set_lapic_nmi_lint(entry[2], *(struct _mps_inti_flags *)(entry + 3), entry[5]);
      break;
    case 10:
      // *** x2APIC local APIC NMI structure ***
      // Describes the local APIC interrupt number that the NMI is
      // connected to on a x2APIC LAPIC. This should be set as an NMI so
      // the system can deal with hardware faults.
      _set_lapic_nmi_lint(((uint32_t *)entry)[1], ((struct _mps_inti_flags *)entry)[1], entry[8]);
      break;
    case 5:
      // *** Local APIC address override structure ***
      // Contains a 64-bit override for the address of the local APIC
      // which should be used instead of the address provided in the
      // table header.
      apic_info.lapic_addr = ((uint64_t *)(entry + 4))[0];
      break;
    }
  }

  if (global_nmi_lint.present) {
    // Apply global NMI to all LAPICs if present
    for (struct apic_lapic *x = apic_info.lapic; x; x = x->next) {
      x->nmi.present = 1;
      x->nmi.lint = global_nmi_lint.lint;
      x->nmi.polarity = global_nmi_lint.polarity;
      x->nmi.trigger_mode = global_nmi_lint.trigger_mode;
    }
  }
}
// }}}

// {{{ APIC

static inline void _write_apic_reg(uint16_t reg, uint32_t val) {
  // APIC register map physical origin = 0xFEE00000
  uint32_t *vreg = (uint32_t *)(0xFEE00000 + reg + PHYSICAL_MAP_OFFSET);
  *vreg = val;
}

static inline uint32_t _read_apic_reg(uint16_t reg) {
  // APIC register map physical origin = 0xFEE00000
  uint32_t *vreg = (uint32_t *)(0xFEE00000 + reg + PHYSICAL_MAP_OFFSET);
  return *vreg;
}

uint8_t apic_check_support(void) {
  uint32_t val;

  __asm__ volatile(
      "movl $0x01, %%eax    \n\t"
      "cpuid               \n\t"
      : "=d" (val)
      :
      : "eax", "ebx", "ecx"
  );

  return !!(val & 0x200);
}

#define PIC1_CMD 0x20
#define PIC1_DAT 0x21
#define PIC2_CMD 0xA0
#define PIC2_DAT 0xA1

void apic_enable(void) {
  // First, disable the classic 8259 PIC
  // Despite the fact that the IRQs will be masked, it is still
  // possible for spurious IRQs to be received on IRQ 7 and 15. For this
  // reason, we still need to remap the IRQs so we don't confuse them
  // with exceptions. Blame IBM.

  _outb(PIC1_CMD, 0x11); // Init
  _outb(PIC2_CMD, 0x11);

  _outb(PIC1_DAT, 32); // Offset IRQs by 32
  _outb(PIC2_DAT, 40);

  _outb(PIC1_DAT, 4); // Inform master of slave
  _outb(PIC2_DAT, 2); // Inform slave it's a slave

  _outb(PIC1_DAT, 1); // Set to 8086 mode
  _outb(PIC2_DAT, 1);

  // IRQs remapped
  // Now mask them

  _outb(PIC2_DAT, 0xFF);
  _outb(PIC1_DAT, 0xFF);

  // Enable APIC, map spurious IRQs to interrupt 255
  _write_apic_reg(0xF0, 0x1FF);
}

struct apic_lapic *apic_get_current(void) {
  uint32_t lapic_id;

  __asm__ volatile(
      "movl $0x1B, %%ecx      \n\t"
      "rdmsr                  \n\t"
      "bt $10, %%ecx          \n\t"
      "jc 1f                  \n\t"
      "  movl $1, %%eax       \n\t"
      "  cpuid                \n\t"
      "  shrl $24, %%ebx      \n\t"
      "  andl $0xFF, %%ebx    \n\t"
      "  movl %%ebx, %0       \n\t"
      "  jmp 2f               \n\t"
      "1:                     \n\t"
      "  movl $0x802, %%ecx   \n\t"
      "  rdmsr                \n\t"
      "  movl %%eax, %0       \n\t"
      "2:"
    : "=r"(lapic_id)
    :: "rax", "rbx", "rcx", "rdx"
  );

  struct apic_lapic *x = apic_info.lapic;
  do {
    if (x->lapic_id == lapic_id) return x;
  } while ((x = x->next));

  return NULL;
}

void apic_enable_x2apic(void) {
  struct apic_lapic *lapic = apic_get_current();
  if (!lapic) return;

  uint8_t success = 0;
  __asm__ volatile(
      "movl $1, %%eax         \n\t"
      "cpuid                  \n\t"
      "bt $21, %%ecx          \n\t"
      "jnc 1f                 \n\t"
      "  movl $0x1B, %%ecx    \n\t"
      "  rdmsr                \n\t"
      "  or $(1<<10), %%eax   \n\t"
      "  wrmsr                \n\t"
      "  movb $1, %0          \n\t"
      "1:"
    : "+r"(success)
    :: "rax", "rbx", "rcx", "rdx"
  );

  if (success) lapic->mode = MODE_X2APIC;
}
// }}}
