#include <kernel.h>
#include <asm.h>
#include <memlayout.h>
#include <irq.h>
#include <page.h>
#include <sys.h>

#define IOREGSEL 0x00
#define IOWIN 0x10
#define IOEOI 0x40

#define IOAPICID 0x00
#define IOAPICVER 0x01
#define IOREDTBL(_n) (0x10 + 2 * (_n))

#define REDIR_MASK (1 << 16)

volatile void *ioapic = NULL;
static int maxredir;

static u32
ioapicrd(u32 reg)
{
  *(volatile u32 *)(ioapic + IOREGSEL) = reg;
  return *(volatile u32 *)(ioapic + IOWIN);
}

static void
ioapicwr(u32 reg, u32 val)
{
  *(volatile u32 *)(ioapic + IOREGSEL) = reg;
  *(volatile u32 *)(ioapic + IOWIN) = val;
}

int
ioapicen(int pin)
{
  if (!ioapic)
    return -1;
  if (pin > maxredir)
    return -1;
  ioapicwr(IOREDTBL(pin), pin + 32);
  ioapicwr(IOREDTBL(pin) + 1, 0);
  return pin + 32;
}

void
ioapicinit(void)
{
  u32 ver;
  ioapic = (volatile void*)iomap(0xFEC00000, 0x1000);
  ver = ioapicrd(IOAPICVER);
  maxredir = (ver >> 16) & 0xff;
  for (u32 i = 0; i <= maxredir; i++) {
    ioapicwr(IOREDTBL(i), REDIR_MASK);
    ioapicwr(IOREDTBL(i) + 1, 0);
  }
}
