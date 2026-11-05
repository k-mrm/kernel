#ifndef _PROC_H
#define _PROC_H

#include <kernel.h>
#include <asm.h>
#include <list.h>
#include <sys.h>
#include <lock.h>

enum procstate {
  NULLSTATE,
  NEWBORN,
  RUNNING,
  READY,
  BLOCKED,
  DEAD,
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
  int yield;
};

struct defunct {
  uint pid;
  int exitcode;
  list ent;
};

struct proc {
  uint pid;
  procstate state;
  char name[16];
  page *kstackpage;
  inode *cwd;
  file *openf[16];
  list ptable;
  list rq;
  list wq;
  pvm *vm;
  ulong sp;
  cpu *cpu;
  cpu *prevcpu;
  proc *parent;
  spinlock lk;
  uint nchild;
  list exitq;
  waitpoint wait;
  int exitcode;
  int (*kf)(void *ka);
  void *ka;
  trapframe *tf;
  uint lockdepth;
  u64 lockflags;
};

void swtch(ulong *prev_rsp, ulong *next_rsp);

#endif
