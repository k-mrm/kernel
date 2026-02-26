#ifndef _X86_IOAPIC_H
#define _X86_IOAPIC_H

#include <irq.h>
#include <kernel.h>

void ioapicinit(u32 id, u32 addr, u32 gsi);
void ioapic_route(u32 pin, u32 vector, u32 dest);

extern struct irqchip ioapic_chip;

#endif // _X86_IOAPIC_H
