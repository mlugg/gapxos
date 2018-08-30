#include "basic_lib.h"

char *itoa(int64_t val, char *str, uint8_t base) {
  if (base < 2 || base > 36) {
    *str = 0;
    return str;
  }

  char *ptr = str;

  if (val < 0) {
    *ptr++ = '-';
    val = -val;
  }

  char *low = ptr;

  do {
    *ptr++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[val % base];
    val /= base;
  } while (val);

  *ptr-- = 0;

  while (low < ptr) {
    char tmp = *low;
    *low++ = *ptr;
    *ptr-- = tmp;
 }

  return str;
}
