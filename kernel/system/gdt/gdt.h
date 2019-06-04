#pragma once

#include <stdint.h>

void gdt_init(void);
uint64_t gdt_add_tss(void *int_stack);
void gdt_get_info(uint16_t *limit, uint64_t *base, uint16_t *krn_code);
