#pragma once
#include <stdint.h>

void cpu_init_ap_payload(void);
void cpu_free_ap_payload(void);
void cpu_send_ipi(uint8_t vector, uint8_t mode, uint32_t dest);
uint8_t cpu_start(uint32_t lapic_id);
void cpu_enable_interrupts(void);
