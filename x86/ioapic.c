#include <kernel.h>
#include <irq.h>
#include <device.h>
#include <vm.h>
#include <kalloc.h>
#include <string.h>
#include "arch.h"
#include "mm.h"
#include "apic.h"

#define KPREFIX     "ioapic:"

#include <printk.h>

#define IOREGSEL        0x00
#define IOWIN           0x10
#define IOEOI           0x40

#define IOAPICID        0x00
#define IOAPICVER       0x01
#define IOREDTBL(_n)    (0x10 + 2 * (_n))

#define REDIR_MASK      (1 << 16)

struct ioapic
{
  volatile void *base;
  u64 pbase;
  u32 id;
  u32 maxredir;
};

struct ioapic_irq
{
  struct ioapic *ioapic;
  int irqno;      
};

static struct ioapic ioapic0;

static u32
ioapicread(struct ioapic *ioa, u32 reg)
{
  *(volatile u32 *)(ioa->base + IOREGSEL) = reg;
  return *(volatile u32 *)(ioa->base + IOWIN);
}

static void
ioapicwrite(struct ioapic *ioa, u32 reg, u32 val)
{
  *(volatile u32 *)(ioa->base + IOREGSEL) = reg;
  *(volatile u32 *)(ioa->base + IOWIN) = val;
}

static int
ioapic_ack(struct irq *irq)
{
  return -1;
}

static void
ioapic_eoi(struct irq *irq)
{
  struct ioapic *ioa = &ioapic0;

  ioapicwrite(ioa, IOEOI, irq->irqno);
}

static int
ioapic_enirq(struct irq *irq)
{
  struct ioapic *ioa = &ioapic0;
  struct ioapic_irq *ioirq = irq->priv;
  int pin = ioirq->irqno;

  if (pin > ioa->maxredir)
    return -1;

  ioapicwrite(ioa, IOREDTBL(pin), irq->irqno);
  ioapicwrite(ioa, IOREDTBL(pin) + 1, 0);

  return 0;
}

static int
ioapic_disirq(struct irq *irq)
{
  return -1;
}

static int
ioapic_probe(struct device *dev)
{
  struct ioapic *ioa = &ioapic0;
  u32 ver;

  ioa->base = devmmap(ioa->pbase, PAGESIZE);
  if (!ioa->base) {
    warn("mmap failed\n");
    return -1;
  }

  ver = ioapicread(ioa, IOAPICVER);
  ioa->maxredir = (ver >> 16) & 0xff;

  log("IOAPIC%d ver %x maxredir %d\n", ioa->id, ver & 0xff, ioa->maxredir);

  for (u32 i = 0; i <= ioa->maxredir; i++) {
    ioapicwrite(ioa, IOREDTBL(i), REDIR_MASK);
    ioapicwrite(ioa, IOREDTBL(i) + 1, 0);
  }

  return probe_irqchip(dev);
}

static struct irq *
ioapic_new_irq(struct irqchip *ic, struct device *dev, int irqno, int (*handler)(struct irq *irq))
{
  struct ioapic *ioa = &ioapic0;
  struct ioapic_irq *ioirq;
  struct irq *irq;

  ioirq = zalloc();
  if (!ioirq)
    return NULL;

  ioirq->irqno = irqno;

  irq = newirq(dev, ic, -1, handler, ioirq);
  if (!irq)
    return NULL;

  return irq;
}

static struct driver ioapic_drv = {
  .name           = "IOAPIC",
  .description    = "I/O APIC Driver",
  .probe          = ioapic_probe,
  .suspend        = NULL,
  .resume         = NULL,
  .param          = "",
};

struct irqchip ioapic_chip = {
  .parent = &lapic_chip,
  .new_irq = ioapic_new_irq,
  .ack = ioapic_ack,
  .eoi = ioapic_eoi,
  .enable_irq = ioapic_enirq,
  .disable_irq = ioapic_disirq,
};

void
ioapicinit(u32 id, u32 addr, u32 gsi)
{
  struct ioapic *ioa = &ioapic0;

  ioa->pbase = addr;
  ioa->id = id;

  new_device(&ioapic_chip.dev, "irqchip", "IOAPIC", &ioapic_drv, NULL);
}
