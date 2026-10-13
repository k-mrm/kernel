#ifndef _WAIT_H
#define _WAIT_H

#include <kernel.h>
#include <proc.h>
#include <lock.h>

struct waitpoint {
  spinlock lk;
  proc *p;
};

#endif
