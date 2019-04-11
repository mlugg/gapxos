.code64
.section .text

# The new GDT - we don't use the one created by the loader just in case it is overwritten.
# This will later be replaced again so we can add Task State Segments, but for now we just need a working one.
# This is essentially identical to the GDT loaded by the loader.

gdt:
gdt_null:        #   Null descriptor
.word 0          # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0          # Access
.byte 1          # Granularity / flags
.byte 0          # Base (31:24)
gdt_data:        #   Data descriptor
.word 0          # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0b10010010 # Access
.byte 0          # Granularity / flags
.byte 0          # Base (31:24)
gdt_code:        #   Code descriptor
.word 0          # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (24:16)
.byte 0b10011010 # Access (exec/read)
.byte 0b00100000 # 64-bit flag
.byte 0          # Base (31:24)
gdt_end:

gdt_ptr:
  gdt_limit: .word gdt_end - gdt  # Limit (length)
  gdt_base: .quad 0               # Base


# Loads the new GDT
.global load_initial_gdt
.type load_initial_gdt, @function
load_initial_gdt:
  lea gdt(%rip), %rax
  movq %rax, gdt_base(%rip)
  lgdt gdt_ptr(%rip)
  popq %rax
  pushq $(gdt_code - gdt)
  pushq %rax
  retfq

