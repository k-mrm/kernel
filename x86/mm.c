#include <kernel.h>
#include <sysmem.h>
#include <vm.h>
#include <kalloc.h>
#include <proc.h>
#include <panic.h>
#include "arch.h"
#include "mm.h"
#include "seg.h"

/* Kernel Page Directory */
static PTE kpml4[512] ALIGNED (PAGESIZE);

extern PTE __boot_pml4[];
extern PTE __boot_pdpt[];

void
switchvm(struct vm *vm)
{
  ulong pgtpa;

  if (!vm)
    panic("null vas");

  pgtpa = V2P(vm->pgdir);
  if (vm->user)
    tss.rsp0 = (ulong)vm->proc->kstack + PAGESIZE;

  asm volatile ("mov %0, %%cr3" :: "r"(pgtpa));
}

void
x86initkvm(struct vm *kvm)
{
  kvm->pgdir       = kpml4;
  kvm->level       = 4;
  kvm->lowestlevel = 1;
}

void
killbootmap (void)
{
  __boot_pml4[PIDX (4, KERNLINK_PA)] = 0;
}
