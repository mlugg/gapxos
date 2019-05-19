#pragma once

#include <stdint.h>

uint8_t has_apic(void);
void disable_pic(void);
void enable_apic(void);
void enable_interrupts(void);
