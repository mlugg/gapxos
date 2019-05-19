#pragma once

#include <stdint.h>

void init_gdt(void);
uint64_t add_tss(void *int_stack);
void get_gdt_info(uint16_t *limit, uint64_t *base, uint16_t *krn_code);
