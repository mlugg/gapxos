#include "system.h"
#include <stdint.h>
#include "../output/display.h"

// At this point, all boot info has been dealt with. Physical memory management is set up.
void system_main(void *acpi, uint32_t acpi_version) {
  print("Hi!\n");
  __asm__("hlt");

  // We should never get here
}
