#ifndef _LOCK_H
#define _LOCK_H

#include <kernel.h>
#include <sys.h>

struct spinlock {
  uint lock;
  ulong flags;
  int holder;
};

#endif
