#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <lock.h>
#include <proc.h>

bool
trylock(spinlock *lk)
{
  ulong flags;
  uint c;
  flags = irqsave();
  c = lapicid();
  if (lk->holder == c)
    panic("recursive");
  if (cmpxchg(&lk->lock, 0, 1) != 0) {
    irqrestore(flags);
    return false;
  }
  lk->holder = c;
  lk->flags = flags;
  return true;
}

void
lock(spinlock *lk)
{
  ulong flags;
  cpu *c;
  flags = irqsave();
  c = lapicid();
  if (lk->holder == c)
    panic("recursive");
  while (cmpxchg(&lk->lock, 0, 1) != 0)
    ;
  lk->holder = c;
  lk->flags = flags;
}

void
unlock(spinlock *lk)
{
  ulong flags;
  flags = lk->flags;
  lk->holder = -1;
  asm volatile ("" ::: "memory");
  lk->lock = 0;
  irqrestore(flags);
}

void
slockinit(spinlock *lk)
{
  lk->lock = 0;
  lk->flags = 0;
  lk->holder = -1;
}
