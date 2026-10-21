#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <lock.h>
#include <proc.h>

bool
trylock(spinlock *lk)
{
  proc *p = myproc();
  cpu *c;
  u64 flags = irqsave();
  c = mycpu();
  if (lk->holder == c)
    panic("recursive");
  if (p) {
    if (!p->lockdepth)
      p->lockflags = flags;
    p->lockdepth++;
  }
  if (cmpxchg(&lk->lock, 0, 1) != 0) {
    if (p)
      p->lockdepth--;
    irqrestore(flags);
    return false;
  }
  lk->holder = c;
  if (!p)
    irqrestore(flags);
  return true;
}

uint
locked(spinlock *lk)
{
  int l;
  u64 flags;
  flags = irqsave();
  l = lk->lock && mycpu() == lk->holder;
  irqrestore(flags);
  return l;
}

void
lock(spinlock *lk)
{
  proc *p = myproc();
  cpu *c;
  u64 flags = irqsave();
  c = mycpu();
  if (lk->holder == c)
    panic("recursive");
  if (p) {
    if (!p->lockdepth)
      p->lockflags = flags;
    p->lockdepth++;
  }
  while (cmpxchg(&lk->lock, 0, 1) != 0)
    ;
  lk->holder = c;
  if (!p)
    irqrestore(flags);
}

void
unlock(spinlock *lk)
{
  proc *p;
  u64 flags;
  lk->holder = NULL;
  asm volatile ("" ::: "memory");
  lk->lock = 0;
  p = myproc();
  if (p) {
    flags = p->lockflags;
    if (!p->lockdepth)
      panic("unlock");
    p->lockdepth--;
    if (!p->lockdepth)
      irqrestore(flags);
  }
}

void
mlock(mutex *m)
{
  lock(&m->lk);
  while (m->lock)
    sleep(&m->wait, &m->lk);
  m->lock = 1;
  m->holder = myproc();
  unlock(&m->lk);
}

void
munlock(mutex *m)
{
  lock(&m->lk);
  m->lock = 0;
  m->holder = NULL;
  wakeup(&m->wait);
  unlock(&m->lk);
}

void
mtxinit(mutex *mtx)
{
  slockinit(&mtx->lk);
  waitinit(&mtx->wait);
  mtx->lock = 0;
  mtx->holder = NULL;
}

void
slockinit(spinlock *lk)
{
  lk->lock = 0;
  lk->holder = NULL;
}
