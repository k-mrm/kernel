#include <kernel.h>
#include <panic.h>
#include <timer.h>
#include <sysmem.h>
#include <irq.h>
#include <device.h>
#include <kalloc.h>
#include <cpu.h>
#include <string.h>
#include "arch.h"
#include "apic.h"

#define KPREFIX   "apic:"

#include <printk.h>

#define ID        0x020
#define VER       0x030
#define TPR       0x080
#define EOI       0x0b0
#define SPIV      0x0f0
#define SPIV_APIC_ENABLED (1 << 8)
#define ESR       0x280
#define ICR_LOW   0x300
#define ICR_HIGH  0x310
#define LVT_TIMER 0x320
#define LVT_TIMER_INT_MASK      (1 << 16)
#define LVT_TIMER_ONE_TIME      (0 << 17)
#define LVT_TIMER_PERIODIC      (1 << 17)
#define LVT_TIMER_TSC_DEADLINE  (2 << 17)

#define TM_INIT     0x380
#define TM_CURRENT  0x390
#define TM_DIV      0x3e0

static struct apic *localapic;

static int
apic_timer_freq(struct apic *apic)
{
  u32 cnt, cnt2;

  cnt = apic->ops->read(apic, TM_CURRENT);
  msleep(1000);
  cnt2 = apic->ops->read(apic, TM_CURRENT);

  if (cnt <= cnt2) {
    warn("lapic timer?");
    return -1;
  }

  apic->freq = cnt - cnt2;
  trace("lapic timer %x -> %x freq:%d\n", cnt, cnt2, apic->freq);
  return 0;
}

static int
apictimer_probe(struct device *device)
{
  struct eventtimer *et = dev_eventtimer(device);
  struct apic *apic = container_of(et, struct apic, timer);
  struct irq *irq;
  int err;
  u32 lvt = 0;

  lvt |= LVT_TIMER_INT_MASK;
  lvt |= 0x40;

  apic->ops->write(apic, LVT_TIMER, lvt);
  // Enable Timer
  apic->ops->write(apic, TM_INIT, 0xffffffff);

  err = apic_timer_freq(apic);
  if (err)
    return -1;

  log("new apictimer!! %d\n", apic->id);

  irq = device->irqchip->new_irq(device->irqchip, device, -1, eventtimerirq);
  if (!irq) {
    warn ("no irq\n");
    return -1;
  }

  lvt = LVT_TIMER_PERIODIC;
  lvt |= irq->irqno;    // periodic mode, vector is irq->irqno

  apic->ops->write(apic, LVT_TIMER, lvt);
  apic->ops->write(apic, TM_INIT, apic->freq / 10);    // 100ms

  return probe_evtimer(device);
}

static void
apictimer_resume(struct device *device)
{
  struct eventtimer *et = dev_eventtimer(device);
  struct apic *apic = container_of(et, struct apic, timer);
  u32 lvt;

  lvt = apic->ops->read(apic, LVT_TIMER);
  lvt &= ~LVT_TIMER_INT_MASK;   // MASK bit
  apic->ops->write(apic, LVT_TIMER, lvt);
}

static void
apictimer_suspend(struct device *device)
{
  struct eventtimer *et = dev_eventtimer(device);
  struct apic *apic = container_of(et, struct apic, timer);
  u32 lvt;

  lvt = apic->ops->read(apic, LVT_TIMER);
  lvt |= LVT_TIMER_INT_MASK;    // MASK bit
  apic->ops->write(apic, LVT_TIMER, lvt);
}

static int
apic_ack(struct irq *irq)
{
  return -1;
}

static void
apic_eoi(struct irq *irq)
{
  struct irqchip *ic = irq->chip;
  struct apic *apic = localapic;

  apic->ops->write(apic, EOI, 0);
}

static uint
apictimergetperiod(struct eventtimer *et)
{
  struct apic *apic = container_of(et, struct apic, timer);
  return apic->periodms;
}

static void
apictimersetperiod(struct eventtimer *et, uint ms)
{
  struct apic *apic = container_of(et, struct apic, timer);
  apic->periodms = ms;
}

static int
apictimer_irq(struct eventtimer *et, struct irq *irq)
{
  return 0;
}

static void
enable_apic(struct device *dev)
{
  struct irqchip *ic = dev_irqchip(dev);
  struct apic *apic = localapic;
  u32 spiv;

  spiv = apic->ops->read(apic, SPIV);
  spiv |= SPIV_APIC_ENABLED;
  apic->ops->write(apic, SPIV, spiv);
}

static void
disable_apic(struct device *dev)
{
  struct irqchip *ic = dev_irqchip(dev);
  struct apic *apic = localapic;
  u32 spiv;

  spiv = apic->ops->read(apic, SPIV);
  spiv &= ~SPIV_APIC_ENABLED;
  apic->ops->write(apic, SPIV, spiv);
}

static void 
setspiv(struct apic *apic)
{
  u32 spiv;
  spiv = 0x20;    // sprious interrupt vector is 0x20
  apic->ops->write(apic, SPIV, spiv);
}

static int
apic_enable_irq(struct irq *irq)
{
  return 0;
}

static int
apic_disable_irq(struct irq *irq)
{
  return 0;
}

static int
apic_probe(struct device *dev)
{
  struct irqchip *ic = container_of(dev, struct irqchip, dev);
  struct apic *apic = localapic;

  if (apic->ops->probe(dev, apic) < 0)
    return -1;

  // Set Sprious Interrupt Vector
  setspiv(apic);
  // Clear Error Status
  apic->ops->write(apic, ESR, 0);
  apic->ops->write(apic, TPR, 0);

  enable_apic(dev);

  return probe_irqchip(dev);
}

static struct irq *
apic_new_irq(struct irqchip *ic, struct device *dev, int irqno, int (*handler)(struct irq *irq))
{
  return newirq(dev, ic, irqno, handler, NULL);
}

static struct driver apic_et_drv = {
  .name           = "lapictimer",
  .description    = "LAPICTimer driver",
  .probe          = apictimer_probe,
  .suspend        = apictimer_suspend,
  .resume         = apictimer_resume,
  .param          = "",
};

static struct eventtimer_if apic_et_ops = {
  .getperiod      = apictimergetperiod,
  .setperiod      = apictimersetperiod,
  .irqhandler     = apictimer_irq,
};

static struct driver apic_irqchip_drv = {
  .name           = "apic",
  .description    = "apic driver",
  .probe          = apic_probe,
  .suspend        = disable_apic,
  .resume         = enable_apic,
  .param          = "xapic,x2apic",
};

struct irqchip lapic_chip = {
  .parent = NULL,
  .new_irq = apic_new_irq,
  .ack = apic_ack,
  .eoi = apic_eoi,
  .enable_irq = apic_enable_irq,
  .disable_irq = apic_disable_irq,
};

void
apicinit(u32 id, struct apic_if *ops)
{
  struct cpu *cpu = mycpu();
  struct apic *apic;
  int rc;
  if (id != 0)
    panic("unimpl");

  apic = alloc();
  if (!apic)
    return;
  apic->ops = ops;
  apic->id = id;

  localapic = apic;

  rc = new_device(&lapic_chip.dev, "irqchip", "LAPIC", &apic_irqchip_drv, NULL);
  if (rc < 0)
    return;

  rc = new_device(&apic->timer.dev, "eventtimer", "LAPICTimer", &apic_et_drv, &cpu->devtree);
  if (rc < 0)
    return;
  apic->timer.ops = &apic_et_ops;
  apic->timer.dev.irqchip = &lapic_chip;

  return;
}
