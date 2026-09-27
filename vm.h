#ifndef _VM_H
#define _VM_H

#include <kernel.h>
#include <sys.h>
#include <page.h>

struct procvm {
  proc *p;
  ulong *cr3;
  struct {
    void *base;
    uint size;
  } stack;
  struct {
    void *base;
    uint size;
  } heap;
};

#endif
