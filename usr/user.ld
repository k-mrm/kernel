OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(main)

SECTIONS
{
  . = 0x1000;
  .text   : { *(.text .text.*) *(.rodata .rodata.*) }
  . = ALIGN(0x1000);
  .data   : { *(.data .data.*) }
  . = ALIGN(0x1000);
  .bss    : { *(.bss .bss.*) *(COMMON) }
}
