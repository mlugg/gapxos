#pragma once

unsigned char ap_init_payload[] = {
                                        //   .code16
                                        //   start:
  0xbe, 0xad, 0xde,                     //     movw $0xdead, %si                    # Will be replaced by the kernel with a pointer to a data structure
  0xc6, 0x44, 0x16, 0x01,               //     movb $0x1, 0x16(%si)                 # Set AP-started flag
                                        //     wait_continue:
  0x80, 0x7c, 0x16, 0x00,               //       cmpb $0x0, 0x16(%si)               # Wait for kernel to unset AP-started flag before continuing
  0x75, 0xfa,                           //     jne wait_continue
  0x66, 0x8b, 0x04,                     //     movl (%si), %eax                     # Get the PML4T address
  0x0f, 0x22, 0xd8,                     //     movl %eax, %cr3                      # Put it into %cr3
  0x0f, 0x20, 0xe0,                     //     movl %cr4, %eax
  0x66, 0x83, 0xc8, 0x20,               //     orl $0x20, %eax                      # Set the PAE bit in %cr4
  0x0f, 0x22, 0xe0,                     //     movl %eax, %cr4
  0x66, 0xb9, 0x80, 0x00, 0x00, 0xc0,   //     movl $0xC0000080, %ecx               # Get the EFER MSR
  0x0f, 0x32,                           //     rdmsr
  0x66, 0x0d, 0x00, 0x09, 0x00, 0x00,   //     orl $0x900, %ecx                     # Set the LM and NX bits
  0x0f, 0x30,                           //     wrmsr
  0x0f, 0x20, 0xc0,                     //     movl %cr0, %eax                      # Get %cr0
  0x66, 0x0d, 0x01, 0x00, 0x00, 0x80,   //     orl $0x80000001, %eax                # Set the PF and PM bits
  0x0f, 0x22, 0xc0,                     //     movl %eax, %cr0
  0x81, 0x44, 0x0a, 0x45, 0x00,         //     addw $(x64_stub - start), 0xa(%si)   # Set the far jump address to the x64_stub address
  0x0f, 0x01, 0x54, 0x04,               //     lgdtw 0x4(%si)                       # Load the temporary x64 GDT
  0xff, 0x6c, 0x0a,                     //     ljmp *0xa(%si)                       # Jump to 64-bit code
                                        //   .code64
                                        //   x64_stub:
  0x66, 0x89, 0xf0,                     //     movw %si, %ax                        # The higher bits of %rsi might not be empty - clear it
  0x48, 0x31, 0xf6,                     //     xorq %rsi, %rsi
  0x66, 0x89, 0xc6,                     //     movw %ax, %si                        # Move the kernel data structure address back into %rsi
  0x48, 0x8b, 0x46, 0x0e,               //     movq 0xe(%rsi), %rax                 # Get the kernel address in %rax
  0xff, 0xe0,                           //     jmpq *%rax                           # Jump to the kernel
};

#define ap_init_payload_len (sizeof ap_init_payload)
