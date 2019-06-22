#include "cpu.h"
#include "ap_init_payload.h"
#include "apic.h"
#include "../../util/memcpy.h"
#include "../mem_mgmt/pmm.h"
#include "../../display.h"
#include "../mem_mgmt/vmm/mem_mgmt.h"
#include "../system.h"

// {{{ Timing

static void _outb(uint16_t port, uint8_t val) {
  __asm__ volatile(
      "outb %0, %1"
      :: "r"(val), "r"(port)
  );
}

static uint8_t _inb(uint16_t port) {
  uint8_t val;
  __asm__ volatile(
      "inb %1, %0"
      : "=r"(val)
      : "d"(port)
  );
  return val;
}

static void _pit_set_timeout(unsigned ms) {
  unsigned cycles = ms * 1193;
  _outb(0x43, 0x30);
  _outb(0x40, cycles & 0xff);
  _outb(0x40, (cycles >> 8) & 0xff);
}

static uint8_t _pit_timeout_reached(void) {
  _outb(0x43, 0xE2);
  return _inb(0x40) & (1<<7);
}

// }}}

// {{{ AP startup

struct ap_data {
  uint32_t pml4t;
  uint16_t tmp_gdt_limit;
  uint32_t tmp_gdt_base;
  uint16_t start_addr;
  uint16_t code_selector;
  uint64_t krn_entry;
  uint8_t has_started;
  uint64_t stack;
} __attribute__((packed));

static void _ap_entry_point(void) {
  print("Hello from processor #2!\n");
  __asm__("hlt");
}

void cpu_init_ap_payload(void) {
  uint64_t payload_location = pmm_ap_low_page;
  uint64_t payload_data = payload_location + ap_init_payload_len;
  uint64_t payload_gdt = payload_data + sizeof (struct ap_data);

  memcpy((void *)(payload_location + PHYSICAL_MAP_OFFSET), ap_init_payload, ap_init_payload_len);


  *((uint16_t *)(payload_location + 1)) = payload_data; // Inject data address into payload at offset 1

  struct ap_data *data = (struct ap_data *)(payload_data + PHYSICAL_MAP_OFFSET);


  uint64_t *temp_gdt = (uint64_t *)(payload_gdt + PHYSICAL_MAP_OFFSET);

  temp_gdt[0] = 0x000100000000ffff;  // Null
  temp_gdt[1] = 0x0000920000000000;  // Data
  temp_gdt[2] = 0x00af9a000000ffff;  // Code

  uint64_t pml4t;
  __asm__ volatile(
    "movq %%cr3, %0"
    : "=r"(pml4t)
  );

  data->pml4t = pml4t;
  data->start_addr = payload_location;
  data->krn_entry = (uint64_t)&_ap_entry_point;
  data->tmp_gdt_base = payload_gdt;
  data->tmp_gdt_limit = 23;
  data->code_selector = 16;
  data->has_started = 0;
}

void cpu_free_ap_payload(void) {
  pmm_free_page((uint64_t)pmm_ap_low_page);
}

static void _cpu_started(struct ap_data volatile *data) {
  data->stack = (uint64_t) malloc_pages(&kern_vmm, 4, 1, 0, 1);
  data->has_started = 0;
}

uint8_t cpu_start(uint32_t lapic) {
  struct ap_data volatile *data = (struct ap_data volatile *)(pmm_ap_low_page + ap_init_payload_len + PHYSICAL_MAP_OFFSET);
  uint8_t page_number = pmm_ap_low_page / 4096;

  cpu_send_ipi(0, 5, lapic); // INIT IPI

  _pit_set_timeout(10); // Wait 10ms
  while (!_pit_timeout_reached());
  
  cpu_send_ipi(page_number, 6, lapic); // SIPI

  _pit_set_timeout(1); // Poll for AP to start with timeout of 1ms
  while (!_pit_timeout_reached()) {
    if (data->has_started) {
      _cpu_started(data);
      return 0;
    }
  }

  cpu_send_ipi(page_number, 6, lapic); // SIPI

  _pit_set_timeout(1000); // Poll for AP to start with timeout of 10ms
  while (!_pit_timeout_reached()) {
    if (data->has_started) {
      _cpu_started(data);
      return 0;
    }
  }

  // AP has not started
  return 1;
}

// }}}

// {{{ IPI

void cpu_send_ipi(uint8_t vector, uint8_t mode, uint32_t dest) {
  uint8_t is_x2apic;
  __asm__ volatile(
      "xorb %0, %0      \n\t"
      "movl $1, %%eax   \n\t"
      "cpuid            \n\t"
      "btl $21, %%ecx   \n\t"
      "adcb $0, %0"
    : "=r"(is_x2apic)
    :: "rax", "rbx", "rcx", "rdx"
  );

  uint32_t cmd = vector | ((mode & 7) << 8) | 0x4000;

  if (is_x2apic) {
    __asm__ volatile(
        "movl $0x830, %%ecx   \n\t"
        "wrmsr"
      :: "a"(cmd), "d"(dest)
      : "rcx"
    );
  } else {
    uint32_t volatile *lapic_base = (uint32_t volatile *)(apic_info.lapic_addr + PHYSICAL_MAP_OFFSET);
    lapic_base[196] = dest << 24;
    lapic_base[192] = cmd;
    while (lapic_base[192] & 0x800); // Wait until interrupt accepted by target
  }
}

// }}}
