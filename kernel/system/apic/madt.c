#include "madt.h"
#include "processors.h"
#include "../mem_mgmt/heap.h"
#include "../mem_mgmt/pmm.h"

static struct lapic *lapic_head;
static struct io_apic *io_apic_head;
static struct irq_gsi_map *irq_gsi_map_head;
static struct nmi_source *nmi_source_head;

struct local_apic_flags {
  uint32_t enabled:1;
  uint32_t online_capable:1;
  uint32_t _reserved:30;
};

struct mps_inti_flags {
  uint16_t polarity:2;
  uint16_t trigger_mode:2;
  uint16_t _reserved:12;
};

void register_lapic(uint32_t proc_uid, uint32_t apic_id, struct local_apic_flags flags, enum lapic_mode mode) {
  // Check if an entry already exists in the list
  for (struct lapic *x = lapic_head; x; x = x->next) {
    if (x->proc_uid == proc_uid) {
      x->lapic_id = apic_id;
      x->mode = mode;
      x->enabled = flags.enabled;
      x->online_capable = flags.online_capable;
      return;
    }
  }

  // Create an entry
  struct lapic *new = malloc(sizeof *new);
  new->next = lapic_head;
  lapic_head = new;

  new->proc_uid = proc_uid;
  new->lapic_id = apic_id;
  new->mode = mode;
  new->enabled = flags.enabled;
  new->online_capable = flags.online_capable;
  new->nmi.present = 0;
}

uint32_t ioapic_read_reg(struct io_apic *io_apic, uint8_t reg) {
  uint32_t volatile *addr = (uint32_t volatile *)(PHYSICAL_MAP_OFFSET + io_apic->paddr);
  addr[0] = reg;
  return addr[4];
}

void ioapic_write_reg(struct io_apic *io_apic, uint8_t reg, uint32_t val) {
  uint32_t volatile *addr = (uint32_t volatile *)(PHYSICAL_MAP_OFFSET + io_apic->paddr);
  addr[0] = reg;
  addr[4] = val;
}

void register_ioapic(uint8_t id, uint32_t addr, uint32_t gsi_base) {
  // Create an entry
  struct io_apic *new = malloc(sizeof *new);
  new->next = io_apic_head;
  io_apic_head = new;

  new->id = id;
  uint32_t volatile *vaddr = (uint32_t volatile *)(PHYSICAL_MAP_OFFSET + addr);
  vaddr[0] = 1;
  new->gdi_count = (vaddr[4] >> 16) & 0xff;
  new->paddr = addr;
  new->gsi_base = gsi_base;
}

void register_iso(uint8_t irq, uint32_t gsi, struct mps_inti_flags flags) {
  // Create an entry
  struct irq_gsi_map *new = malloc(sizeof *new);
  new->next = irq_gsi_map_head;
  irq_gsi_map_head = new;

  new->gsi = gsi;
  new->irq = irq;
  new->polarity = flags.polarity;
  new->trigger_mode = flags.trigger_mode;
}

void register_nmi_source(struct mps_inti_flags flags, uint32_t gsi) {
  // Create an entry
  struct nmi_source *new = malloc(sizeof *new);
  new->next = nmi_source_head;
  nmi_source_head = new;

  new->gsi = gsi;
  new->polarity = flags.polarity;
  new->trigger_mode = flags.trigger_mode;
}

struct {
  uint8_t present:1;
  uint8_t lint:1;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;
} global_nmi_lint;

void set_lapic_nmi_lint(uint32_t proc_uid, struct mps_inti_flags flags, uint8_t lint_no) {
  if (proc_uid == 0xFF) {
    // Applies to all CPUs
    global_nmi_lint.present = 1;
    global_nmi_lint.lint = lint_no;
    global_nmi_lint.polarity = flags.polarity;
    global_nmi_lint.trigger_mode = flags.trigger_mode;
    return;
  }
  // Check if an entry already exists in the list
  for (struct lapic *x = lapic_head; x; x = x->next) {
    if (x->proc_uid == proc_uid) {
      x->nmi.present = 1;
      x->nmi.lint = lint_no;
      x->nmi.polarity = flags.polarity;
      x->nmi.trigger_mode = flags.trigger_mode;
      return;
    }
  }

  // Create an entry
  struct lapic *new = malloc(sizeof *new);
  new->next = lapic_head;
  lapic_head = new;

  new->proc_uid = proc_uid;
  new->nmi.present = 1;
  new->nmi.lint = lint_no;
  new->nmi.polarity = flags.polarity;
  new->nmi.trigger_mode = flags.trigger_mode;
}

static inline void next_entry(uint8_t **cur) {
  *cur += (*cur)[1];
}

void parse_madt(struct madt *madt) {
  uint64_t lapic_addr = madt->lapic_addr;
  
  uint8_t *end = (uint8_t *)madt + madt->hdr.length;

  for (uint8_t *entry = (uint8_t *)(madt + 1); entry < end; next_entry(&entry)) {
    switch (entry[0]) {
    case 0:
      // *** Legacy local APIC structure ***
      // Describes a single local APIC for a processor operating in
      // legacy mode with a 1-byte ID.
      register_lapic(entry[2], entry[3], ((struct local_apic_flags *)entry)[1], MODE_LEGACY_APIC);
      break;
    case 9:
      // *** x2APIC local APIC structure ***
      // Describes a single local APIC for a processor operating in
      // x2APIC mode with a 4-byte ID.
      register_lapic(((uint32_t *)entry)[1], ((uint32_t *)entry)[2], ((struct local_apic_flags *)entry)[3], MODE_X2APIC);
      break;
    case 1:
      // *** I/O APIC structure ***
      // Describes a single I/O APIC. Specifies the 1-byte ID, the
      // 32-bit physical address of the APIC's registers, and the global
      // system interrupts it is responsible for.
      register_ioapic(entry[2], (((uint32_t *)entry)[1]), ((uint32_t *)entry)[2]);
      break;
    case 2:
      // *** Interrupt source override structure ***
      // Contains a mapping between the 8259 PIC IRQs and the APIC
      // interrupts. The IRQs are assumed to be identity mapped to the
      // APIC interrupts; however, most systems will have one or more
      // exceptions to this rule, which are conveyed through this
      // structure.
      register_iso(entry[3], ((uint32_t *)entry)[1], ((struct mps_inti_flags *)entry)[4]);
      break;
    case 3:
      // *** NMI source structure ***
      // Specifies a Global System Interrupt which should be set as
      // non-maskable. It is equivalent to a classical NMI, indicating a
      // hardware fault that the OS needs to deal with.
      register_nmi_source(((struct mps_inti_flags *)entry)[1], ((uint32_t *)entry)[1]);
      break;
    case 4:
      // *** Legacy local APIC NMI structure ***
      // Describes the local APIC interrupt number that the NMI is
      // connected to on a legacy LAPIC. This should be set as an NMI so
      // the system can deal with hardware faults.
      set_lapic_nmi_lint(entry[2], *(struct mps_inti_flags *)(entry + 3), entry[5]);
      break;
    case 10:
      // *** x2APIC local APIC NMI structure ***
      // Describes the local APIC interrupt number that the NMI is
      // connected to on a x2APIC LAPIC. This should be set as an NMI so
      // the system can deal with hardware faults.
      set_lapic_nmi_lint(((uint32_t *)entry)[1], ((struct mps_inti_flags *)entry)[1], entry[8]);
      break;
    case 5:
      // *** Local APIC address override structure ***
      // Contains a 64-bit override for the address of the local APIC
      // which should be used instead of the address provided in the
      // table header.
      lapic_addr = ((uint64_t *)(entry + 4))[0];
      break;
    }
  }

  if (global_nmi_lint.present) {
    // Apply global NMI to all LAPICs if present
    for (struct lapic *x = lapic_head; x; x = x->next) {
      x->nmi.present = 1;
      x->nmi.lint = global_nmi_lint.lint;
      x->nmi.polarity = global_nmi_lint.polarity;
      x->nmi.trigger_mode = global_nmi_lint.trigger_mode;
    }
  }

  set_madt_info(
      lapic_head,
      io_apic_head,
      irq_gsi_map_head,
      nmi_source_head,
      lapic_addr
  );
}
