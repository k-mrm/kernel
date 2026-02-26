#include <kernel.h>
#include <cpu.h>
#include <kalloc.h>

#define KPREFIX   "cpu:"

#include <printk.h>

static ROOT(cputree);

static void *
find_cpu(struct tree *node, void *arg)
{
  struct cpu *cpu = TREE_ENTRY(node, struct cpu, cn); 
  uint id = (uint)(ulong)arg;

  if (cpu->cpuid == id)
    return cpu;
  else
    return NULL;
}

struct cpu *
mycpu(void)
{
  return tree_dfs(&cputree, find_cpu, (void *)(ulong)0 /* tmp */);
}

struct cpu *
getcpu(int id)
{
  return tree_dfs(&cputree, find_cpu, (void *)(ulong)id);
}

void
initcpu(int cpuid)
{
  struct cpu *cpu = zalloc();

  if (!cpu)
    return;
  cpu->cpuid = cpuid;
  inittree(&cpu->cn);
  new_child(&cputree, &cpu->cn);
  inittree(&cpu->devtree);
}
