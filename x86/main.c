#include <kernel.h>
#include <printk.h>
#include <multiboot.h>
#include <acpi.h>
#include <cpu.h>
#include <kalloc.h>
#include "arch.h"
#include "com.h"
#include "mm.h"
#include "pci.h"

void NORETURN kernelmain (void);

void NORETURN
x86main (MultiBootInfo *mb)
{
	killbootmap();
	serialportinit();
	mb2parsebootinfo(mb);
	// In x86-64, First 1MB is reserved
	sysrsrvmem (0x0, 0x100000);
	reservekernelarea();
	gdtinit();
	x86cpuinit();
	x86trapinit();
	initpic8259a();
        // Kernel early mapping is 0-1GiB
        kallocinitearly(0x0, 1 * GiB);
	initcpu(0);
	acpiinit();

	kernelmain();
}
