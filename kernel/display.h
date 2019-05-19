#pragma once

#include <stdint.h>

enum kdisp_color {
  BLACK,
  BLUE,
  GREEN,
  CYAN,
  RED,
  PURPLE,
  BROWN,
  GRAY,
  DARK_GRAY,
  LIGHT_BLUE,
  LIGHT_GREEN,
  LIGHT_CYAN,
  LIGHT_RED,
  LIGHT_PURPLE,
  YELLOW,
  WHITE,
};

void init_display(uint16_t *, uint32_t, uint32_t);
void kdisp_color(enum kdisp_color fg, enum kdisp_color bg);
void kdisp_putc(char c);
void kdisp_setxy(uint32_t x, uint32_t y);
void kdisp_getxy(uint32_t *x, uint32_t *y);
void kdisp_getwh(uint32_t *w, uint32_t *h);
void print(char *x);
