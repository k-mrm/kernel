#include <cpu.h>
#include <device.h>
#include <fs.h>
#include <kalloc.h>
#include <kernel.h>
#include <module.h>
#include <panic.h>
#include <pci.h>
#include <printk.h>
#include <proc.h>
#include <syscall.h>
#include <timer.h>
#include <x86/arch.h>

void apmain(void) NORETURN;

void NORETURN kernelmain(void) {
  kernelmap();
  kallocinit();
  ramdiskinit();
  dev_probe("irqchip");
  dev_probe("console");
  dev_probe("timer");
  dev_probe("eventtimer");
  dev_probe("block");
  dev_probe("irq");
  initmodule();
  initfs();
  initprocess();
  syscallinit();
  apmain();
}

void NORETURN apmain(void) {
  lsdev();

  INTR_ENABLE;

  for (;;)
    HLT;
  panic("kernelmain exits");
}
