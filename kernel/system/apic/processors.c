#include "processors.h"
#include "ap_init_payload.h"
#include "../mem_mgmt/pmm.h"
#include "../../util/memcpy.h"
#include "../../display.h"

static struct {
  struct lapic *lapic;
  struct io_apic *io_apic;
  struct irq_gsi_map *irq_gsi_map;
  struct nmi_source *nmi_source;
  uint64_t lapic_addr;
} info;

void set_madt_info(struct lapic *lapic, struct io_apic *io_apic, struct irq_gsi_map *irq_gsi_map, struct nmi_source *nmi_source, uint64_t lapic_addr) {
  info.lapic = lapic;
  info.io_apic = io_apic;
  info.irq_gsi_map = irq_gsi_map;
  info.nmi_source = nmi_source;
  info.lapic_addr = lapic_addr;
}

struct ap_data {
  uint32_t pml4t;
  uint16_t tmp_gdt_limit;
  uint32_t tmp_gdt_base;
  uint16_t start_addr;
  uint16_t code_selector;
  uint64_t krn_entry;
  uint8_t has_started;
} __attribute__((packed));

extern void ap_entry_point(void);

void init_ap_payload(void) {
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
  data->krn_entry = (uint64_t)&ap_entry_point;
  data->tmp_gdt_base = payload_gdt;
  data->tmp_gdt_limit = 23;
  data->code_selector = 16;
  data->has_started = 0;
}

void free_ap_payload(void) {
  free_phys_page((void *)(uint64_t)pmm_ap_low_page);
}

struct lapic *get_current_cpu(void) {
  uint32_t lapic_id;

  __asm__ volatile(
      "movl $0x1B, %%ecx      \n\t"
      "rdmsr                  \n\t"
      "bt $10, %%ecx          \n\t"
      "jc 1f                  \n\t"
      "  movl $1, %%eax       \n\t"
      "  cpuid                \n\t"
      "  shrl $24, %%ebx      \n\t"
      "  andl $0xFF, %%ebx    \n\t"
      "  movl %%ebx, %0       \n\t"
      "  jmp 2f               \n\t"
      "1:                     \n\t"
      "  movl $0x802, %%ecx   \n\t"
      "  rdmsr                \n\t"
      "  movl %%eax, %0       \n\t"
      "2:"
    : "=r"(lapic_id)
    :: "rax", "rbx", "rcx", "rdx"
  );

  struct lapic *x = info.lapic;
  do {
    if (x->lapic_id == lapic_id) return x;
  } while ((x = x->next));

  return NULL;
}

void attempt_x2apic_enable(void) {
  struct lapic *lapic = get_current_cpu();
  if (!lapic) return;

  uint8_t success = 0;
  __asm__ volatile(
      "movl $1, %%eax         \n\t"
      "cpuid                  \n\t"
      "bt $21, %%ecx          \n\t"
      "jnc 1f                 \n\t"
      "  movl $0x1B, %%ecx    \n\t"
      "  rdmsr                \n\t"
      "  or $(1<<10), %%eax   \n\t"
      "  wrmsr                \n\t"
      "  movb $1, %0          \n\t"
      "1:"
    : "+r"(success)
    :: "rax", "rbx", "rcx", "rdx"
  );

  if (success) lapic->mode = MODE_X2APIC;
}

void send_ipi(uint8_t vector, uint8_t mode, uint32_t dest_lapic) {
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
      :: "a"(cmd), "d"(dest_lapic)
      : "rcx"
    );
  } else {
    uint32_t volatile *lapic_base = (uint32_t volatile *)(info.lapic_addr + PHYSICAL_MAP_OFFSET);
    lapic_base[196] = dest_lapic << 24;
    lapic_base[192] = cmd;
    while (lapic_base[192] & 0x800); // Wait until interrupt accepted by target
  }
}


void outb(uint16_t port, uint8_t val) {
  __asm__ volatile(
      "outb %0, %1"
      :: "r"(val), "r"(port)
  );
}

uint8_t inb(uint16_t port) {
  uint8_t val;
  __asm__ volatile(
      "inb %1, %0"
      : "=r"(val)
      : "d"(port)
  );
  return val;
}

void pit_set_timeout(unsigned ms) {
  unsigned cycles = ms * 1193;
  outb(0x43, 0x30);
  outb(0x40, cycles & 0xff);
  outb(0x40, (cycles >> 8) & 0xff);
}

uint8_t pit_timeout_reached(void) {
  outb(0x43, 0xE2);
  return inb(0x40) & (1<<7);
}

uint8_t start_cpu(uint32_t lapic_id) {
  struct ap_data volatile *data = (struct ap_data volatile *)(pmm_ap_low_page + ap_init_payload_len + PHYSICAL_MAP_OFFSET);
  uint8_t page_number = pmm_ap_low_page / 4096;

  send_ipi(0, 5, lapic_id); // INIT IPI

  pit_set_timeout(10); // Wait 10ms
  while (!pit_timeout_reached());
  
  send_ipi(page_number, 6, lapic_id); // SIPI

  pit_set_timeout(1); // Poll for AP to start with timeout of 1ms
  while (!pit_timeout_reached()) {
    if (data->has_started) {
      data->has_started = 0;
      return 0;
    }
  }

  send_ipi(page_number, 6, lapic_id); // SIPI

  pit_set_timeout(1000); // Poll for AP to start with timeout of 10ms
  while (!pit_timeout_reached()) {
    if (data->has_started) {
      data->has_started = 0;
      return 0;
    }
  }

  print("NOTE: Failed to init AP");

  return 1;
}
