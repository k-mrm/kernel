#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>
#include <page.h>

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
      p = kalloc();
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
  cr3page = kalloc();
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
  printk("f\n");
  wrcr3(kcr3pa);
}
