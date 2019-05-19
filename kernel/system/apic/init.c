#include <stdint.h>
#include "../mem_mgmt/pmm.h"

// Returns 1 if the system supports APIC, 0 otherwise
uint8_t has_apic(void) {
  uint32_t val;

  __asm__ volatile(
      "movl $0x01, %%eax    \n\t"
      "cpuid               \n\t"
      : "=d" (val)
      :
      : "eax", "ebx", "ecx"
  );

  return !!(val & 0x200);
}

static inline void outb(uint16_t port, uint8_t value) {
  __asm__ volatile(
      "outb %0, %1"
      :
      : "a" (value), "Nd" (port)
  );
}

#define PIC1_CMD 0x20
#define PIC1_DAT 0x21
#define PIC2_CMD 0xA0
#define PIC2_DAT 0xA1

// Disables the classic 8259 PIC
void disable_pic(void) {
  // Despite the fact that the IRQs will be masked, it is still
  // possible for spurious IRQs to be received on IRQ 7 and 15. For this
  // reason, we still need to remap the IRQs so we don't confuse them
  // with exceptions. Blame IBM.

  outb(PIC1_CMD, 0x11); // Init
  outb(PIC2_CMD, 0x11);

  outb(PIC1_DAT, 32); // Offset IRQs by 32
  outb(PIC2_DAT, 40);

  outb(PIC1_DAT, 4); // Inform master of slave
  outb(PIC2_DAT, 2); // Inform slave it's a slave

  outb(PIC1_DAT, 1); // Set to 8086 mode
  outb(PIC2_DAT, 1);

  // IRQs remapped
  // Now mask them

  outb(PIC2_DAT, 0xFF);
  outb(PIC1_DAT, 0xFF);
}

static inline void write_apic_reg(uint16_t reg, uint32_t val) {
  // APIC register map physical origin = 0xFEE00000
  uint32_t *vreg = (uint32_t *)(0xFEE00000 + reg + PHYSICAL_MAP_OFFSET);
  *vreg = val;
}

static inline uint32_t read_apic_reg(uint16_t reg) {
  // APIC register map physical origin = 0xFEE00000
  uint32_t *vreg = (uint32_t *)(0xFEE00000 + reg + PHYSICAL_MAP_OFFSET);
  return *vreg;
}

// Enables the APIC
void enable_apic(void) {
  write_apic_reg(0xF0, 0x1FF); // Enable APIC and map spurious IRQs to interrupt 255
}

// Allows the CPU to accept interrupts
void enable_interrupts(void) {
  __asm__ volatile("sti");
}
