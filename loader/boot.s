.set MAGIC, 0xe85250d6  # Magic number
.set ARCH, 0  # This means x86 protected mode
.set HEADER_LENGTH, multiboot_header_end - multiboot_header
.set CHECKSUM, 1<<32 - MAGIC - ARCH - HEADER_LENGTH

.align 8

multiboot_header:

.long MAGIC
.long ARCH
.long HEADER_LENGTH
.long CHECKSUM

.align 8

# Mark end

.word 0
.word 0
.long 8

multiboot_header_end:



.section .bss
.align 16
stack_bottom:
.skip 65546 # 64 KiB
stack_top:

# Allocate space for page structures
.align 0x1000
page_structs:
.skip 4194304 # 4MiB

.code32
.section .text

gdt32:
gdt32_null:      #   Null descriptor
.word 0xffff     # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0          # Access
.byte 1          # Granularity
.byte 0          # Base (31:24)
gdt32_data:      #   Data descriptor
.word 0xffff     # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0b10010010 # Access
.byte 0xC0       # Granularity
.byte 0          # Base (31:24)
gdt32_code:      #   Code descriptor
.word 0xffff     # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0b10011010 # Access
.byte 0xC0       # Granularity
.byte 0          # Base (31:24)
gdt32_end:

gdt32_pointer:
.word gdt32_end - gdt32  # Limit (length)
.long gdt32              # Base


.global start
.type start, @function
start: # Entry point
  cli                # Fuck interrupts
  lgdt (gdt32_pointer)
  mov $stack_top, %esp # Set up a stack
  mov $stack_top, %ebp

  # Push args in reverse order
  push $65536 # Stack size
  push $stack_bottom
  push $page_structs
  push %ebx   # Pointer to multiboot info structure
  call lmain # Loader


# Called from C. Takes 1 argument, pointer to paging structures
.global setup_longmode
.type setup_longmode, @function
setup_longmode: # Sets up long mode
  push %ebx
  pushfl
  pushfl
  pop %eax
  xorl $(1<<21), %eax # Toggles bit 21
  push %eax
  popfl
  pushfl
  pop %eax
  mov (%esp), %ebx
  popfl
  cmp %eax, %ebx
  je err # CPUID not supported
  pop %ebx

  mov $0x80000000, %eax
  cpuid
  cmp $0x80000001, %eax
  jb err # Extended CPUID not supported

  mov $0x80000001, %eax
  cpuid
  test $(1<<29), %edx # Tests bit 29
  jz err # Long mode not supported
  # Long mode supported!

  # Disable paging
  mov %cr0, %eax
  and $(1<<31 - 1), %eax # Wipe bit 31
  mov %eax, %cr0

  # Set up page tables
  mov 4(%esp), %eax # Set destination register to 0x1000
  mov %eax, %cr3 # Set page location to dest. register

  # Enable PAE
  push %edi
  mov %cr4, %edi
  or $(1<<5), %edi # Set bit 5
  mov %edi, %cr4
  pop %edi

  # Switch to compatibility mode (IA32e)
  mov $0xC0000080, %ecx
  rdmsr
  or $(1<<8), %eax
  or $(1<<11), %eax
  wrmsr

  # Enable paging again
  mov %cr0, %eax
  or $(1<<31), %eax
  or $1, %eax
  mov %eax, %cr0

  # All done!
  ret

  # When we get an error trying to switch to long mode
  err:
  # Maybe output an error message?
  hlt


gdt64:
gdt64_null:      #   Null descriptor
.word 0xffff     # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0          # Access
.byte 1          # Granularity
.byte 0          # Base (31:24)
gdt64_data:      #   Data descriptor
.word 0          # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (23:16)
.byte 0b10010010 # Access
.byte 0          # Granularity
.byte 0          # Base (31:24)
gdt64_code:      #   Code descriptor
.word 0xffff     # Limit (15:0)
.word 0          # Base (15:0)
.byte 0          # Base (24:16)
.byte 0b10011010 # Access (exec/read)
.byte 0b10101111 # Granularity, 64-bit flag, limit (19:16)
.byte 0          # Base (31:24)
gdt64_end:

gdt64_pointer:
.word gdt64_end - gdt64  # Limit (length)
.long gdt64              # Base


# Nearly there! Just register GDT and go to actual long mode
.global load_kernel
.type load_kernel, @function
load_kernel:
mov %esp, %ebp
#mov 8(%ebp), %esi

lgdt (gdt64_pointer)

ljmp $(gdt64_code - gdt64), $long_start


.code64
# We're in long mode; jump to the loaded kernel
long_start:

cli
mov $(gdt64_data - gdt64), %ax
mov %ax, %ss
mov %ax, %ds
mov %ax, %es

movq 4(%esp), %rax  # Entry point

# Now esp is at the start of the value
movl 12(%esp), %edi  # Pointer to mb structure

movl 16(%esp), %esi  # Pointer to krn_start
movl 20(%esp), %edx  # Pointer to krn_end

movq 24(%esp), %rcx # Pointer to bottom of stack (physical)
movq 40(%esp), %r8 # Length of stack


movq 32(%esp), %rbx # Pointer to bottom of stack (virtual)

mov %rbx, %rsp
add $4095, %rsp
mov %rsp, %rbp

jmp *%rax
