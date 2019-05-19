#include "display.h"
#include <stdint.h>
#include <stddef.h>

// This whole file is temporary, it's just how to output for now

struct {
  uint16_t *text;
  uint32_t width;
  uint32_t height;
  uint32_t row;
  uint32_t col;
  uint8_t color;
} text_info;

void init_display(uint16_t *p_text, uint32_t p_width, uint32_t p_height) {
  text_info.text = p_text;
  text_info.width = p_width;
  text_info.height = p_height;
}

void kdisp_color(enum kdisp_color fg, enum kdisp_color bg) {
  text_info.color = bg | (fg << 4);
}

void kdisp_putc(char c) {
  size_t idx = text_info.row * text_info.width + text_info.col++;
  text_info.text[idx] = c + (text_info.color << 8);
  if (text_info.row >= text_info.width) {
    ++text_info.col;
    text_info.row -= text_info.width;
  }
}

void kdisp_setxy(uint32_t x, uint32_t y) {
  text_info.col = x;
  text_info.row = y;
}

void kdisp_getxy(uint32_t *x, uint32_t *y) {
  if (x) *x = text_info.col;
  if (y) *y = text_info.row;
}

void kdisp_getwh(uint32_t *w, uint32_t *h) {
  if (w) *w = text_info.width;
  if (h) *h = text_info.height;
}

void print(char *x) {
  for (; *x; ++x) {
    if (*x == '\n') {
      uint32_t y;
      kdisp_getxy(NULL, &y);
      kdisp_setxy(0, y + 1);
    } else {
      kdisp_putc(*x);
    }
  }
}
