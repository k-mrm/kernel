#include <kernel.h>
#include <irq.h>
#include <kalloc.h>
#include <panic.h>

#define KPREFIX     "irq:"

#include <printk.h>

static void *
getirqchip(struct device *dev, void *_)
{
	return dev;
}

struct irqchip *
myirqchip(void)
{
	struct irqchip *ic;

	ic = dev_traverse_cpu("irqchip", getirqchip, NULL);

	if (!ic)
		panic("ic!?");
	return ic;
}

static struct irq *
getirq(int irqno)
{
	struct irqchip *irqchip = myirqchip();
	struct irq *irq;

	if (irqchip) {
		LIST_FOREACH (irq, &irqchip->irqs, in) {
			if (irq->irqno == irqno)
				return irq;
		}
	}

	irqchip = dev_traverse("irqchip", getirqchip, NULL);

	if (irqchip) {
		LIST_FOREACH (irq, &irqchip->irqs, in) {
			if (irq->irqno == irqno)
				return irq;
		}
	}

	return NULL;
}

int
newirq(struct device *dev, struct irqchip *ic, int irqno, bool priv, int (*handler)(struct irq *irq))
{
        struct irq *irq = alloc();
        if (!irq)
                return -1;
        if (irqno < 0)
                return -1;

	list_add(&ic->irqs, &irq->in);

	irq->chip = ic;
        irq->irqno = irqno;
        irq->device = dev;
        irq->handler = handler;

        return 0;
}

int
handleirq(int irqno)
{
        struct irq *irq = getirq(irqno);
        int ret;

        if (!irq)
                return -1;

        irq->chip->ops->ack(irq);
        irq->chip->ops->eoi(irq);

        ret = irq->handler(irq);
        return ret;
}

int
probe_irqchip(struct device *dev)
{
        struct irqchip *irqchip = dev_irqchip(dev);
	initlist(&irqchip->irqs);
        log ("using irqchip: %s\n", dev->name);
        return 0;
}
