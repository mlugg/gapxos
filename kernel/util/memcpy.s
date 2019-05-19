.text
.global memcpy
.type memcpy, @function
memcpy:
  movq %rdx, %rcx
  cld
  rep movsb
  movq %rdi, %rax
  ret
