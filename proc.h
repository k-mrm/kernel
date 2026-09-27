#ifndef _PROC_H
#define _PROC_H

#include <kernel.h>
#include <list.h>
#include <sys.h>

enum procstate {
  DEAD,
  NEWBORN,
  RUNNING,
  READY,
  BLOCKED,
  ZOMBIE,
};

struct context {
  ulong r15, r14, r13, r12, rbx, rbp, rip;
} PACKED;

struct cpu {
  ulong sched;
  proc *current;
  list cpulist;
  uint id;
};

struct proc {
  uint pid;
  procstate state;
  char name[16];
  page *kstackpage;
  list ptable;
  list rq;
  // spinlock lock
  procvm *vm;
  ulong sp;
  cpu *prevcpu;
  int (*kf)(void *ka);
  void *ka;
};

void swtch(ulong *prev_rsp, ulong *next_rsp);

#endif
