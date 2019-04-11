#include "display.h"
#include <stdint.h>

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

void set_color(uint8_t fg, uint8_t bg) {
  text_info.color = bg | (fg << 4);
}

void print(char *string) {
  if (!text_info.text) return;

  uint16_t *text = text_info.text;
  uint32_t width = text_info.width;
  uint32_t height = text_info.height;
  uint8_t color = text_info.color;
  uint32_t row = text_info.row;
  uint32_t col = text_info.col;

  do {
    if (*string == '\n') {
      row = (row + 1) % height;
      col = 0;
      continue;
    }

    *(text + col + row*width) = *string + (color << 8);
    col = (col + 1) % width;
    if (!col)
      row = (row + 1) % height;
  } while (*++string);

  text_info.row = row;
  text_info.col = col;
}

void cls() {
  text_info.row = text_info.col = 0;
  for (int i = 0; i < text_info.height; ++i)
    for (int j = 0; j < text_info.width; ++j)
      *(text_info.text + j + i*text_info.width) = text_info.color << 8;
}
