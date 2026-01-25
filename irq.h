#ifndef _IRQ_H
#define _IRQ_H

#include <kernel.h>
#include <device.h>

struct irq;

struct irqchip_if {
        void (*eoi)(struct irq *irq);
        int (*ack)(struct irq *irq);
};

struct irqchip
{
	struct device dev;
	struct irqchip_if *ops;

	struct list irqs;
};

#define dev_irqchip(_d)	container_of(_d, struct irqchip, dev)

struct irq
{
        int irqno;
        struct irqchip *chip;
        struct device *device;

	struct list in;

        int (*handler)(struct irq *irq);
        int (*enable)(struct irq *irq);
        int (*disable)(struct irq *irq);
};

struct irqchip *myirqchip(void);
int newirq(struct device *dev, struct irqchip *ic, int irqno, bool priv, int (*handler)(struct irq *irq));
int handleirq(int irqno);
int probe_irqchip(struct device *dev);

#endif  // _IRQ_H
