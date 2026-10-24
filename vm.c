#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>
#include <page.h>
#include <proc.h>
#include <vm.h>
#include <elf.h>
#include <fs.h>

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

static void
vmmappage(ulong virt, ulong phys, ulong flags)
{
  mappages(myproc()->vm->cr3, virt, phys, PAGESIZE, flags);
}

static segment *
vaseg(ulong virt)
{
  pvm *vm;
  if (!myproc())
    return NULL;
  vm = myproc()->vm;
  segment *seg;
  for (seg = vm->seg; seg < vm->seg + 3; seg++)
    if (seg->base <= virt && virt < seg->end)
      return seg;
  return NULL;
}

/*
 *           21          12          0
 *  | ptindex | pt offset | page off |
 *
 */
static page **
ptwalk(segment *seg, ulong off)
{
  pagetree **ppt;
  page *p;
  ppt = &seg->pt[(off >> 21) & 0x3f];
  if (!*ppt) {
    p = allocpage();
    if (!p)
      return NULL;
    *ppt = pageaddress(p);
  }
  return &(*ppt)->pages[(off >> 12) & 0x1ff];
}

bool
validate(ulong addr, uint len, int wr)
{
  segment *seg;
  if (len == 0)
    return false;
  for (;;) {
    seg = vaseg(addr);
    if (!seg || (!seg->w && wr))
      return false;
    if (addr + len > seg->end) {
      len -= seg->end - addr;
      addr = seg->end;
    } else
      break;
  }
  return true;
}

bool
validstr(ulong addr, uint limit, int wr)
{
  for (ulong a = addr; a < addr + limit; a++) {
    if (!validate(a, 1, wr))
      return false;
    if (*((char*)a) == '\0')
      return true;
  }
  return false;
}

int
faultresume(trapframe *tf, ulong addr, int wr, int user)
{
  segment *seg;
  page *p, **pp;
  ulong flags = 0;
  int ret = 0;
  if (user)
    sti();
  else if (tf->rflags & EFLAGS_IF)
    sti();
  seg = vaseg(addr);
  if (!seg) {
    if (myproc()) {
      // sigsegv
      panic("sigsegv %p", addr);
      ret = -1;
      goto end;
    } else {
      panic("kernel pf");
    }
  }
  flags = (seg->w ? PTE_W : 0) |
          PTE_P |
          PTE_U;
  pp = ptwalk(seg, addr - seg->base);
  if (!pp) {
    panic("ppwalk");
    ret = -1;
    goto end;
  }
  if (!*pp) {
    p = allocpage();
    if (!p) {
      ret = -1;
      panic("nopage");
      goto end;
    }
    *pp = p;
  } 
  if (seg->ino) {
    mlock(&seg->ino->mtx);
    if (readi(seg->ino, pageaddress(*pp), seg->offset, PAGESIZE) < 0) {
      munlock(&seg->ino->mtx);
      ret = -1;
      panic("readi");
      goto end;
    }
    munlock(&seg->ino->mtx);
  }
  vmmappage(PAGEALIGNDOWN(addr), pa(pageaddress(*pp)), flags);
end:
  cli();
  return ret;
}

void*
vmap(inode *ino, ulong vaddr, ulong sz, int perm, uint offset)
{
  segment *seg;
  seg = vaseg(vaddr);
  if (!seg) {
    panic("todo");
  }
  if (seg != vaseg(vaddr+sz))
    return NULL;
  if (!seg->ino) {
    seg->ino = idup(ino);
    seg->offset = offset;
  } else if (seg->ino != ino) {
    return NULL;
  }
  return (void*)vaddr;
}

void
vmsegload(segment *seg, ulong uv, page *p)
{
  ulong off;
  page **pp;
  if (uv < seg->base || seg->end <= uv)
    return;
  off = uv - seg->base;
  pp = ptwalk(seg, off);
  if (!pp)
    return;
  *pp = p;
}

void
vmseginit(segment *seg, ulong base, ulong size)
{
  if (!PAGEALIGNED(base) || !PAGEALIGNED(size))
    panic("vmseginit");
  seg->base = base;
  seg->end = base + size;
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
  stext(vm)->r = 1;
  stext(vm)->x = 1;
  sstack(vm)->r = 1;
  sstack(vm)->w = 1;
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
