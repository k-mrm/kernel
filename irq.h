#ifndef _IRQ_H
#define _IRQ_H

#include <device.h>
#include <kernel.h>

struct irq;

struct irqchip {
  struct irqchip *parent;

  struct device dev;

  struct irq *(*new_irq)(struct irqchip *ic, struct device *dev, int irqno,
                         int (*handler)(struct irq *));
  int (*alloc)(struct irqchip *ic);

  void (*eoi)(struct irq *irq);
  int (*ack)(struct irq *irq);
  int (*enable_irq)(struct irq *irq);
  int (*disable_irq)(struct irq *irq);
};

#define dev_irqchip(_d) container_of((_d), struct irqchip, dev)

struct irq {
  int irqno;
  struct device device;
  struct irqchip *chip;

  struct device *dev;

  struct list in;

  void *priv;

  int (*handler)(struct irq *);
};

#define dev_irq(_d) container_of((_d), struct irq, device)

struct irq *newirq(struct device *dev, struct irqchip *ic, int irqno,
                   int (*handler)(struct irq *), void *priv);
int enable_irq(struct irq *irq);
int disable_irq(struct irq *irq);
int handleirq(int irqno);
int probe_irqchip(struct device *dev);
struct device *irq_device(struct irq *irq);

#endif // _IRQ_H
