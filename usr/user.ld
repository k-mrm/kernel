OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(main)

SECTIONS
{
  . = 0x1000 + SIZEOF_HEADERS;
  .text   : { *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data   : { *(.data .data.*) }
  .bss    : { *(.bss .bss.*) *(COMMON) }
}
