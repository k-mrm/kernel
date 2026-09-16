OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(entry16)

virt = 0xffff800000020000;
phys = 0x20000;
offset = virt - phys;

pa_entry32 = ABSOLUTE(entry32 - offset);
pa_trampoline64 = ABSOLUTE(trampoline64 - offset);

SECTIONS {
	. = virt;
	__kstart = .;
	__ktext = .;

	.text : AT(ADDR(.text) - offset) {
		KEEP(*(.text.head))
		KEEP(*(.text.boot))
		. = ALIGN(4096);
		__kinit = .;
		*(.text.init)
		. = ALIGN(4096);
		__kinit_e = .;
		*(.text)
		*(.text.*)
	}

	__ktext_e = .;
	
	. = ALIGN(4096);
	.data : AT(ADDR(.data) - offset) {
		*(.data .data.*)
	}

	. = ALIGN(4096);
	.rodata : AT(ADDR(.rodata) - offset) {
		__rodata = .;
		*(.rodata .rodata.*)
		__rodata_e = .;
	}

	. = ALIGN(4096);
	.boot_pgt : AT(ADDR(.boot_pgt) - offset) {
		__boot_pgt_s = .;
		__boot_pml4 = .;
		. += 4096;
		__boot_pdpt = .;
		. += 4096;
		__boot_pd = .;
		. += 4096;
		__boot_pgt_e = .;
        }

	. = ALIGN(4096);
	.bss : AT(ADDR(.bss) - offset) {
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		__bss_end = .;
	}
	
	. = ALIGN(4096);
	__kend = .;

	/DISCARD/ : {
		*(.comment .comment.*)
	}
}

