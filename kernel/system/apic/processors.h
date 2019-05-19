#pragma once

#include <stdint.h>
#include "madt.h"

enum lapic_mode {
  MODE_LEGACY_APIC,
  MODE_X2APIC,
};

struct lapic {
  uint32_t proc_uid;
  uint32_t lapic_id;
  enum lapic_mode mode;
  uint8_t enabled:1;
  uint8_t online_capable:1;
  struct {
    uint8_t present:1;
    uint8_t lint:1;
    uint8_t polarity:2;
    uint8_t trigger_mode:2;
  } nmi;

  struct lapic *next;
};

struct io_apic {
  uint8_t id;
  uint8_t gdi_count;
  uint32_t paddr;
  uint32_t gsi_base;

  struct io_apic *next;
};

struct irq_gsi_map {
  uint32_t gsi;
  uint8_t irq;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;

  struct irq_gsi_map *next;
};

struct nmi_source {
  uint32_t gsi;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;

  struct nmi_source *next;
};

void set_madt_info(struct lapic *lapic, struct io_apic *io_apic, struct irq_gsi_map *irq_gsi_map, struct nmi_source *nmi_source, uint64_t lapic_addr);
void init_ap_payload(void);
void free_ap_payload(void);
struct lapic *get_current_cpu(void);
void attempt_x2apic_enable(void);
uint8_t start_cpu(uint32_t lapic_id);
