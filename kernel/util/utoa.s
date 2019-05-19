.data
digits: .ascii "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

.text
.global utoa
.type utoa, @function
utoa:
  movq %rdi, %rax
  movq %rdx, %rcx
  movq %rsi, %rdi

  lea digits(%rip), %r9

  next_char:
    xorq %rdx, %rdx
    divq %rcx
    movq (%r9, %rdx, 1), %r8
    movq %r8, (%rdi)
    incq %rdi
    testq %rax, %rax
  jnz next_char

  movq $0, (%rdi)
  decq %rdi

  movq %rsi, %rcx

  reverse:
    movb (%rdi), %dl
    xchgb (%rcx), %dl
    movb %dl, (%rdi)
    decq %rdi
    incq %rcx
    cmpq %rdi, %rcx
  jb reverse

  movq %rsi, %rax
  ret
