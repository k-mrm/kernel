#ifndef _PROC_H
#define _PROC_H

#include <kernel.h>
#include <asm.h>
#include <list.h>
#include <sys.h>
#include <lock.h>

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
  tss ts;
  ulong gdt[NR_SEG];
};

struct proc {
  uint pid;
  procstate state;
  char name[16];
  page *kstackpage;
  inode *cwd;
  list ptable;
  list rq;
  pvm *vm;
  ulong sp;
  cpu *cpu;
  cpu *prevcpu;
  proc *parent;
  waitpoint wait;
  int (*kf)(void *ka);
  void *ka;
  trapframe *tf;
  uint lockdepth;
  u64 lockflags;
};

void swtch(ulong *prev_rsp, ulong *next_rsp);

#endif
