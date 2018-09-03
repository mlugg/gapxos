/*
 * MIT License
 *
 * Copyright (c) 2018 The gapxos Authors

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>

#ifndef ELF64_H
#define ELF64_H

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef uint64_t Elf64_Sxword;

struct Elf64_Ehdr {
  unsigned char e_ident[16]; /* ELF identification */
  Elf64_Half e_type; /* Object file type */
  Elf64_Half e_machine; /* Machine type */
  Elf64_Word e_version; /* Object file version */
  Elf64_Addr e_entry; /* Entry point address */
  Elf64_Off e_phoff; /* Program header offset */
  Elf64_Off e_shoff; /* Section header offset */
  Elf64_Word e_flags; /* Processor-specific flags */
  Elf64_Half e_ehsize; /* ELF header size */
  Elf64_Half e_phentsize; /* Size of program header entry */
  Elf64_Half e_phnum; /* Number of program header entries */
  Elf64_Half e_shentsize; /* Size of section header entry */
  Elf64_Half e_shnum; /* Number of section header entries */
  Elf64_Half e_shstrndx; /* Section name string table index */
};

struct Elf64_Phdr {
  Elf64_Word p_type; /* Type of segment */
  Elf64_Word p_flags; /* Segment attributes */
  Elf64_Off p_offset; /* Offset in file */
  Elf64_Addr p_vaddr; /* Virtual address in memory */
  Elf64_Addr p_paddr; /* Reserved */
  Elf64_Xword p_filesz; /* Size of segment in file */
  Elf64_Xword p_memsz; /* Size of segment in memory */
  Elf64_Xword p_align; /* Alignment of segment */
};

#endif
