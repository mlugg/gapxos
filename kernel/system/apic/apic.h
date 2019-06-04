#pragma once

#include <stdint.h>

enum apic_lapic_mode {
  MODE_LEGACY_APIC,
  MODE_X2APIC,
};

struct apic_lapic {
  uint32_t proc_uid;
  uint32_t lapic_id;
  enum apic_lapic_mode mode;
  uint8_t enabled:1;
  uint8_t online_capable:1;
  struct {
    uint8_t present:1;
    uint8_t lint:1;
    uint8_t polarity:2;
    uint8_t trigger_mode:2;
  } nmi;

  struct apic_lapic *next;
};

struct apic_io_apic {
  uint8_t id;
  uint8_t gdi_count;
  uint32_t paddr;
  uint32_t gsi_base;

  struct apic_io_apic *next;
};

struct apic_irq_map {
  uint32_t gsi;
  uint8_t irq;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;

  struct apic_irq_map *next;
};

struct apic_nmi_source {
  uint32_t gsi;
  uint8_t polarity:2;
  uint8_t trigger_mode:2;

  struct apic_nmi_source *next;
};

struct apic_info {
  struct apic_lapic *lapic;
  struct apic_io_apic *io_apic;
  struct apic_irq_map *irq_gsi_map;
  struct apic_nmi_source *nmi_source;
  uint64_t lapic_addr;
};

extern struct apic_info apic_info;

void apic_parse_madt(void *_madt);
uint8_t apic_check_support(void);
void apic_enable(void);
struct apic_lapic *apic_get_current(void);
void apic_enable_x2apic(void);
