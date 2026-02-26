#include <irq.h>
#include <kalloc.h>
#include <kernel.h>
#include <panic.h>

#define KPREFIX "irq:"

#include <printk.h>

static void *__getirq(struct device *dev, void *arg) {
  struct irq *irq = dev_irq(dev);
  int irqno = (int)arg;

  if (irq->irqno == irqno) {
    return irq;
  } else {
    return NULL;
  }
}

static struct irq *getirq(int irqno) {
  struct irq *irq;

  irq = dev_traverse_cpu("irq", __getirq, (void *)irqno);

  if (irq)
    return irq;

  irq = dev_traverse("irq", __getirq, (void *)irqno);

  return irq;
}

static int allocirq(void) {
  static int no = 0x20;
  return no++;
}

static int irq_dev_enable(struct device *dev) {
  struct irq *irq = dev_irq(dev);
  struct irqchip *ic = irq->chip;

  ic->enable_irq(irq);
}

static struct driver irq_driver = {
    .name = "irq",
    .description = "irq as device",
    .probe = irq_dev_enable,
    .suspend = NULL,
    .resume = NULL,
    .param = "disable",
};

struct device *irq_device(struct irq *irq) {
  return parent_device(&irq->device);
}

struct irq *newirq(struct device *dev, struct irqchip *ic, int irqno,
                   int (*handler)(struct irq *), void *priv) {
  struct irq *irq = alloc();
  if (!irq)
    return NULL;

  irq->chip = ic;
  if (irqno < 0)
    irq->irqno = allocirq();
  else
    irq->irqno = irqno;
  irq->handler = handler;
  irq->priv = priv;

  new_device(&irq->device, "irq", dev->name, &irq_driver, &dev->node);

  return irq;
}

int disable_irq(struct irq *irq) { return -1; }

static int irq_ack(struct irq *irq) {
  struct irqchip *ic = irq->chip;

  for (; ic; ic = ic->parent) {
    if (ic->ack)
      ic->ack(irq);
  }
}

static int irq_eoi(struct irq *irq) {
  struct irqchip *ic = irq->chip;

  for (; ic; ic = ic->parent) {
    if (ic->eoi)
      ic->eoi(irq);
  }
}

int handleirq(int irqno) {
  struct irq *irq = getirq(irqno);
  int ret;

  if (!irq)
    return -1;

  irq_ack(irq);
  irq_eoi(irq);

  ret = irq->handler(irq);
  return ret;
}

int probe_irqchip(struct device *dev) {
  struct irqchip *irqchip = dev_irqchip(dev);
  log("using irqchip: %s\n", dev->name);
  return 0;
}
