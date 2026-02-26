#ifndef _X86_APIC_H
#define _X86_APIC_H

#include <kernel.h>
#include <irq.h>
#include <timer.h>
#include <device.h>

struct apic;

struct apic_if {
  int   (*probe)(struct device *dev, struct apic *apic);
  u32   (*read)(struct apic *apic, u32 reg);
  void  (*write)(struct apic *apic, u32 reg, u32 val);
  void  (*sendipi)(struct apic *apic, int id);
};

struct apic {
  int id;

  uint freq;
  uint periodms;

  struct eventtimer timer;

  struct apic_if *ops;
};

struct apic_hw {
  ;
};

extern struct irqchip lapic_chip;

void xapicinit(uint id);
void x2apicinit(uint id);
void apicinit(uint id, struct apic_if *ops);

#endif    // _X86_APIC_H
