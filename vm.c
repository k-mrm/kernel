#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>
#include <page.h>
#include <proc.h>
#include <vm.h>

static ulong *kcr3;
static ulong kcr3pa;

static ulong *
pagewalk(ulong *pgt, ulong virt, bool alloc)
{
  ulong *pte;
  page *p;
  uint lvl;
  ulong pgtpa;
  for (lvl = 4; lvl > 1; lvl--) {
    pte = &pgt[PIDX(lvl, virt)];
    if (*pte & PTE_P) {
      pgtpa = *pte & PTE_PA_MASK;
      pgt = (ulong*)va(pgtpa);
    } else if (alloc) {
      p = allocpage();
      if (!p)
        return NULL;
      pgt = pageaddress(p);
      pgtpa = pa(pgt);
      *pte = (pgtpa & PTE_PA_MASK) | PTE_P | PTE_W | PTE_U;
    } else
      return NULL;
  }
  return &pgt[PIDX(lvl, virt)];
}

static void
mappages(ulong *pgt, ulong virt, ulong phys, ulong sz, ulong flags)
{
  ulong *pte;
  if (!PAGEALIGNED(virt) || !PAGEALIGNED(phys) || !PAGEALIGNED(sz))
    return;
  for (ulong p = 0; p < sz; p += PAGESIZE, virt += PAGESIZE, phys += PAGESIZE) {
    pte = pagewalk(pgt, virt, true);
    if (!pte)
      panic("mappages");
    *pte = (phys & PTE_PA_MASK) | flags;
  }
}

void *
iomap(ulong base, ulong size)
{
  void *virt;
  virt = va(base);
  mappages(kcr3, (ulong)virt, base, size, PTE_P | PTE_W | PTE_PWT | PTE_PCD);
  return virt;
}

void
kernelmap(void)
{
  ulong base, end;
  ulong flags;
  page *cr3page;
  ulong v;
  cr3page = allocpage();
  if (!cr3page)
    panic("omg");
  kcr3 = pageaddress(cr3page);
  kcr3pa = pa(kcr3);
  for (int i = 0; i < e820count; i++) {
    base = PAGEALIGNDOWN(e820[i].base);
    end = PAGEALIGN(e820[i].base + e820[i].len);
    printk("map %p-%p\n", base, end);
    for (ulong p = base; p < end; p += PAGESIZE) {
      v = (ulong)va(p);
      flags = PTE_P;
      if (IS_KERN_TEXT(v))
        flags |= 0;
      else if (IS_KERN_RODATA(v))
        flags |= 0;
      else
        flags |= PTE_W;
      mappages(kcr3, v, p, PAGESIZE, flags);
    }
  }
  // switch world
  wrcr3(kcr3pa);
}

static segment *
vaseg(void *virt)
{
  proc *pr = myproc();
  segment *seg;
  return seg;
}

void
segload(segment *seg, page *p)
{
  if (!seg->downgrow) {
    lappend(&seg->pagelist, &p->e);
    seg->end += PAGESIZE;
  } else {
    ladd(&seg->pagelist, &p->e);
    seg->base -= PAGESIZE;
  }
}

pvm *
procvm(void)
{
  pvm *vm;
  vm = kmalloc(sizeof *vm);
  if (!vm)
    return NULL;
  vm->cr3page = allocpage();
  if (!vm->cr3page)
    goto failed;
  vm->cr3 = pageaddress(vm->cr3page);
  memcpy(vm->cr3, kcr3, PAGESIZE);
  linit(&stext(vm)->pagelist);
  stext(vm)->r = 1;
  stext(vm)->x = 1;
  stext(vm)->end = stext(vm)->base = 0x1000;
  linit(&sstack(vm)->pagelist);
  sstack(vm)->r = 1;
  sstack(vm)->w = 1;
  sstack(vm)->downgrow = 1;
  sstack(vm)->end = sstack(vm)->base = USTACKTOP;
  return vm;
failed:
  if (vm->cr3page)
    freepage(vm->cr3page);
  kfree(vm);
  return NULL;
}

void
vmswitch(proc *p)
{
  pvm *vm = p->vm;
  ulong flags;
  if (!vm)
    return;
  flags = irqsave();
  mycpu()->ts.rsp0 = (u64)pageaddress(p->kstackpage) + PAGESIZE;
  wrcr3(pa(vm->cr3));
  irqrestore(flags);
}
