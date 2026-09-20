#ifndef _IRQ_H
#define _IRQ_H

#include <kernel.h>
#include <asm.h>
#include <sys.h>

struct irq {
  int irqno;
  void (*eoi)(irq*);
  int (*handle)(irq*);
};

#endif
