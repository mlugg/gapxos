.code64
.section .text

gdt_ptr:
  gdt_limit: .word 0
  gdt_base: .quad 0

.global reload_gdt
.type reload_gdt, @function
reload_gdt:
  movq %rdi, gdt_base(%rip)
  movw %si, gdt_limit(%rip)
  lgdt gdt_ptr(%rip)
  popq %rax
  pushq %rdx
  pushq %rax
  retfq
