#ifndef _PROC_H
#define _PROC_H

#include <kernel.h>

enum procstate {
  RUNNING,
  READY,
  BLOCKED,
  ZOMBIE,
};

struct context {
  u64 rsp;
};

struct stackframe {
  u64 r15, r14, r13, r12, rbx, rbp, rip;
} PACKED;

struct proc {
  int pid;
  procstate state;
  char name[16];
  void *stack;
  int prio;
};

#endif
