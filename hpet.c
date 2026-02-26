#include "hpet.h"
#include <kalloc.h>
#include <kernel.h>
#include <sysmem.h>
#include <timer.h>
#include <vm.h>

#define KPREFIX "HPET:"

#include <printk.h>

#define HPET_MMIO_SIZE 1024

#define HPET_ID 0x0
#define HPET_CLK_PERIOD 0x4
#define HPET_GCR 0x10
#define HPET_MCR 0xf0

struct hpet {
  volatile void *base;
  ulong basepa;

  struct timer timer;

  bool cnt64;
  uint nchannel;
  uint periodfs; // 10(^-15) s
};

static inline void hpetwr32(struct hpet *hpet, ulong offset, u32 val) {
  *(volatile u32 *)(hpet->base + offset) = val;
}

static inline u32 hpetrd32(struct hpet *hpet, ulong offset) {
  return *(volatile u32 *)(hpet->base + offset);
}

static inline u64 hpetrd64(struct hpet *hpet, ulong offset) {
  return *(volatile u64 *)(hpet->base + offset);
}

static void hpetctrl(struct hpet *hpet, bool en) {
  u32 gcr = hpetrd32(hpet, HPET_GCR);

  if (!!(gcr & 1) == en)
    return;

  gcr |= en ? 1 : 0;
  hpetwr32(hpet, HPET_GCR, gcr);
}

static void hpet_resume(struct device *dev) {
  struct timer *tm = container_of(dev, struct timer, dev);
  struct hpet *hpet = container_of(tm, struct hpet, timer);
  hpetctrl(hpet, true);
}

static void hpet_suspend(struct device *dev) {
  struct timer *tm = container_of(dev, struct timer, dev);
  struct hpet *hpet = container_of(tm, struct hpet, timer);
  hpetctrl(hpet, false);
}

#define USEC2FSEC 1000000000
static ulong hpetusec2period(struct timer *tm, uint usec) {
  struct hpet *hpet = container_of(tm, struct hpet, timer);
  ulong fsec = (ulong)usec * USEC2FSEC;
  ulong period = fsec / hpet->periodfs;
  return period;
}

static ulong hpetgetcnt(struct hpet *hpet) { return hpetrd64(hpet, HPET_MCR); }

static ulong hpetgetcntraw(struct timer *tm) {
  struct hpet *hpet = container_of(tm, struct hpet, timer);

  return hpetgetcnt(hpet);
}

static bool hpetdead(struct hpet *hpet) {
  u64 now, after;

  now = hpetgetcnt(hpet);
  for (int i = 0; i < 1000; i++) // busy loop
    ;
  after = hpetgetcnt(hpet);
  return now >= after;
}

static int hpet_probe(struct device *dev) {
  struct timer *tm = container_of(dev, struct timer, dev);
  struct hpet *hpet = container_of(tm, struct hpet, timer);
  u32 id, clkperiod;
  struct iomem *iomem;

  iomem = iomap(dev, hpet->basepa, HPET_MMIO_SIZE);
  if (!iomem)
    return -1;
  hpet->base = iomem->base;

  hpetctrl(hpet, false);

  id = hpetrd32(hpet, HPET_ID);
  clkperiod = hpetrd32(hpet, HPET_CLK_PERIOD);

  hpet->cnt64 = !!(id & (1 << 13));
  hpet->nchannel = (id >> 8) & 0x1f;
  hpet->periodfs = clkperiod;

  hpetctrl(hpet, true);
  log("%s: %d channel(s) clock period: %d ns %d bit counter\n", dev->name,
      hpet->nchannel, clkperiod / 1000000, hpet->cnt64 ? 64 : 32);
  trace("HPET %p\n", hpetgetcnt(hpet));

  if (hpetdead(hpet)) {
    warn("%s: Dead HPET\n", dev->name);
    goto err;
  }
  return probe_timer(dev);

err:
  hpetctrl(hpet, false);
  return -1;
}

static struct driver hpet_driver = {
    .name = "HPET",
    .description = "HPET Timer Device Driver",
    .probe = hpet_probe,
    .suspend = hpet_suspend,
    .resume = hpet_resume,
    .param = "disable",
};

static struct timer_if tmhpet = {
    .read = hpetgetcntraw,
    .usec2period = hpetusec2period,
};

int hpetinit(ulong baseaddr, int n) {
  struct hpet *hpet;
  int rc;

  sysrsrvmem(baseaddr, HPET_MMIO_SIZE);
  hpet = alloc();
  if (!hpet)
    return -1;

  hpet->basepa = baseaddr;
  hpet->timer.ops = &tmhpet;

  rc = new_device(&hpet->timer.dev, "timer", "HPET", &hpet_driver, NULL);
  return rc;
}
