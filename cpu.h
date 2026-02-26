#ifndef _CPU_H
#define _CPU_H

#include <kernel.h>
#include <device.h>
#include <x86/arch.h>
#include <x86/context.h>

struct proc;

struct cpu
{
  uint cpuid;
  struct proc *current;
  struct context context;

  struct tree cn;         // node
  struct tree devtree;    // head
};

struct cpu *mycpu(void);
struct cpu *getcpu(int id);
void initcpu(int cpuid);

#endif  // _CPU_H
