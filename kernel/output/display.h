#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void init_display(uint16_t *, uint32_t, uint32_t);
void set_color(uint8_t, uint8_t);
void print(char *);
void cls();

#endif
