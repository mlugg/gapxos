ENTRY(kernel_main)
KERNEL_VADDR = 0xFFFF800000000000;
SECTIONS
{
  . = KERNEL_VADDR;

  .text :
  {
    _code = .;
    *(.text)
    *(.rodata*)
    . = ALIGN(4096);
  }

  .data :
  {
    _data = .;
    *(.data)
    . = ALIGN(4096);
  }

  .eh_frame :
  {
    _ehframe = .;
    *(.eh_frame)
    . = ALIGN(4096);
  }

  .bss :
  {
    _bss = .;
    *(.bss)
    *(COMMON)
    . = ALIGN(4096);
  }

  _end = .;

  /DISCARD/ :
  {
    *(.comment)
  }
}
