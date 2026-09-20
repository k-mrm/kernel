#include <kernel.h>
#include <asm.h>
#include <memlayout.h>
#include <irq.h>
#include <page.h>
#include <sys.h>

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

volatile void *lapic;

static inline u32
ioread32(volatile void *base, u32 off)
{
  return *(volatile u32 *)(base + off);
}

static inline void
iowrite32(volatile void *base, u32 off, u32 val)
{
  *(volatile u32 *)(base + off) = val;
}

static inline u32
lapicrd(u32 off)
{
  return ioread32(lapic, off);
}

static inline void
lapicwr(u32 off, u32 val)
{
  iowrite32(lapic, off, val);
  ioread32(lapic, ID);
}

static void
lapiceoi(irq *_)
{
  lapicwr(EOI, 0);
}

static u32
lapicbase(void)
{
  ulong base = rdmsr64(IA32_APIC_BASE);
  return (base & IA32_APIC_BASE_APIC_BASE_MASK);
}

static void
enxapic(void)
{
  ulong base = rdmsr64(IA32_APIC_BASE);
  if (base & IA32_APIC_BASE_APIC_GLOBAL_ENABLE)
    return;
  base |= IA32_APIC_BASE_APIC_GLOBAL_ENABLE;
  wrmsr64(IA32_APIC_BASE, base);
}

static void
enlapic(void)
{
  u32 spiv;
  spiv = lapicrd(SPIV);
  spiv |= SPIV_APIC_ENABLED;
  lapicwr(SPIV, spiv);
}

static int
lapictimer_handle(irq *_)
{
  return 0;
}

static void
enlapictimer(void)
{
  u32 lvt;
  lvt = LVT_TIMER_PERIODIC;
  // lvt |= LVT_TIMER_INT_MASK;
  lvt |= 0x40;    // periodic mode, vector is 0x40
  lapicwr(LVT_TIMER, lvt);
  lapicwr(TM_DIV, 0xb);
  lapicwr(TM_INIT, 10000000);
  newirq(0x40, lapiceoi, lapictimer_handle);
}

static void
setspurious(void)
{
  lapicwr(SPIV, 0x20);
  newirq(0x20, lapiceoi, NULL);
}

void
lapicinit(void)
{
  enxapic();
  lapic = (volatile void*)iomap(lapicbase(), 0x1000);
  setspurious();
  lapicwr(EOI, 0);
  // Clear Error Status
  lapicwr(ESR, 0);
  enlapic();
  lapicwr(TPR, 0);
  enlapictimer();
}
