#ifndef _LOCK_H
#define _LOCK_H

#include <kernel.h>
#include <asm.h>
#include <sys.h>

struct spinlock {
  uint lock;
  ulong flags;
  int holder;
} ALIGNED(CACHELINE);

#define SPINLOCK(_l)  spinlock _l = {0, 0, -1}

#endif
