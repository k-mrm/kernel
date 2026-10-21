#ifndef _LOCK_H
#define _LOCK_H

#include <kernel.h>
#include <asm.h>
#include <sys.h>

struct spinlock {
  uint lock;
  cpu *holder;
} ALIGNED(CACHELINE);

struct waitpoint {
  spinlock lk;
  proc *p;
};

#define SPINLOCK(_l)  spinlock _l = {0,NULL}

struct mutex {
  spinlock lk;
  waitpoint wait;
  uint lock;
  proc *holder;
} ALIGNED(CACHELINE);

#endif
