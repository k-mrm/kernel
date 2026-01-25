#include <kernel.h>
#include <panic.h>
#include <printk.h>
#include <kalloc.h>
#include <timer.h>
#include <device.h>
#include <proc.h>
#include <cpu.h>
#include <pci.h>
#include <fs.h>
#include <syscall.h>
#include <x86/arch.h>
#include <module.h>

void NORETURN
kernelmain (void)
{
        kernelmap();
        kallocinit();
        ramdiskinit();
        dev_probe("console");
        dev_probe("irqchip");
        dev_probe("timer");
        dev_probe("eventtimer");
        dev_probe("block");
        initmodule();
        initfs();
        initprocess();
        syscallinit();
        apmain();
}

void NORETURN
apmain (void)
{
        lsdev ();

        INTR_ENABLE;

        for (;;)
                HLT;
        panic ("kernelmain exits");
}
