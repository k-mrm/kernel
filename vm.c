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

static bool segreadonly(segment*);

static ulong *
pagewalk(pvm *vm, ulong virt, bool alloc)
{
  ulong *pgt;
  ulong *pte;
  page *p;
  uint lvl;
  ulong pgtpa;
  if (vm)
    pgt = vm->cr3;
  else
    pgt = kcr3;
  for (lvl = 4; lvl > 1; lvl--) {
    pte = &pgt[PIDX(lvl, virt)];
    if (*pte & PTE_P) {
      pgtpa = *pte & PTE_PA_MASK;
      pgt = (ulong*)va(pgtpa);
    } else if (alloc) {
      p = allocpage();
      if (!p)
        return NULL;
      if (vm)
        lappend(&vm->pgtp, &p->e);
      pgt = pageaddress(p);
      pgtpa = pa(pgt);
      *pte = (pgtpa & PTE_PA_MASK) | PTE_P | PTE_W | PTE_U;
    } else
      return NULL;
  }
  return &pgt[PIDX(lvl, virt)];
}

static void
mappages(pvm *vm, ulong virt, ulong phys, ulong sz, ulong flags)
{
  ulong *pte;
  if (!PAGEALIGNED(virt) || !PAGEALIGNED(phys) || !PAGEALIGNED(sz))
    return;
  for (ulong p = 0; p < sz; p += PAGESIZE, virt += PAGESIZE, phys += PAGESIZE) {
    pte = pagewalk(vm, virt, true);
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
  mappages(NULL, (ulong)virt, base, size, PTE_P | PTE_W | PTE_PWT | PTE_PCD);
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
      mappages(NULL, v, p, PAGESIZE, flags);
    }
  }
  // switch world
  wrcr3(kcr3pa);
}

static void
vmmappage(ulong virt, ulong phys, ulong flags)
{
  mappages(myproc()->vm, virt, phys, PAGESIZE, flags);
}

static segment *
vaseg(ulong virt)
{
  pvm *vm;
  segment *seg;
  if (!myproc())
    return NULL;
  vm = myproc()->vm;
  for (int i = 0; i < NSEG; i++) {
    seg = vm->seg[i];
    if (seg && seg->base <= virt && virt < seg->end)
      return seg;
  }
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

static ulong
fixpage(segment *seg, page **pp, ulong soff, int wr)
{
  page *p, *np;
  ulong flags = PTE_U | PTE_P;
  if (!pp)
    panic("fixpage");
  p = *pp;
  if (!p) {
    p = allocpage();
    *pp = p;
    if (seg->ino) {
      mlock(&seg->ino->mtx);
      if (readi(seg->ino, pageaddress(p), seg->offset+soff, PAGESIZE) < 0) {
        munlock(&seg->ino->mtx);
        return 0;
      }
      munlock(&seg->ino->mtx);
    }
  }
  if (segreadonly(seg))
    return flags;
  if (seg->w && !wr && seg->ref == 1)
    return flags; // CoW
  if (p->ref > 1) {
    np = allocpage();
    if (!np)
      return 0;
    memcpy(pageaddress(np), pageaddress(p), PAGESIZE);
    *pp = np;
    putpage(p);
  }
  return PTE_W | flags;
}

int
faultresume(trapframe *tf, ulong addr, int wr, int user)
{
  segment *seg;
  page **pp;
  ulong soff;
  ulong flags;
  int ret = 0;
  if (user)
    sti();
  else if (tf->rflags & EFLAGS_IF)
    sti();
  seg = vaseg(addr);
  if (!seg) {
    if (myproc()) {
      // sigsegv
      panic("%s: sigsegv %p", myproc()->name, addr);
      ret = -1;
      goto end;
    } else {
      panic("kernel pf");
    }
  }
  if (!seg->w && wr) {
    ret = -1;
    goto end;
  }
  soff = PAGEALIGNDOWN(addr) - seg->base;
  pp = ptwalk(seg, soff);
  if (!pp) {
    ret = -1;
    goto end;
  }
  if ((flags = fixpage(seg, pp, soff, wr)) == 0) {
    ret = -1;
    goto end;
  }
  vmmappage(PAGEALIGNDOWN(addr), pa(pageaddress(*pp)), flags);
end:
  cli();
  return ret;
}

static segment*
allocseg(void)
{
  segment *seg;
  seg = kmalloc(sizeof *seg);
  slockinit(&seg->lk);
  seg->ref = 1;
  return seg;
}

static segment*
newseg(pvm *vm)
{
  for (int i = 0; i < NSEG; i++) {
    if (!vm->seg[i])
      return vm->seg[i] = allocseg();
  }
  panic("fullseg");
}

void*
vmap(inode *ino, ulong vaddr, ulong sz, int perm, uint offset)
{
  segment *seg;
  pvm *vm = myproc()->vm;
  seg = vaseg(vaddr);
  if (!seg) {
    seg = newseg(vm);
    seg->r = perm & VM_READ;
    seg->w = perm & VM_WRITE;
    seg->x = perm & VM_EXEC;
    seg->base = vaddr;
    seg->end = vaddr + PAGEALIGN(sz);
  }
  if (seg != vaseg(vaddr+sz-1))
    return NULL;
  if (!seg->ino) {
    seg->ino = idup(ino);
    seg->offset = offset;
  } else if (seg->ino != ino) {
    return NULL;
  }
  return (void*)vaddr;
}

int
vmload(ulong uv, page *p)
{
  segment *seg;
  ulong off;
  page **pp;
  seg = vaseg(uv);
  if (!seg)
    return -1;
  off = uv - seg->base;
  pp = ptwalk(seg, off);
  if (!pp)
    return -1;
  *pp = p;
  return 0;
}

static bool
segreadonly(segment *seg)
{
  return seg->r && !seg->w;
}

void
vmtextinit(ulong vaddr, ulong size)
{
  segment *seg;
  pvm *vm;
  if (!PAGEALIGNED(vaddr) || !PAGEALIGNED(size))
    panic("align");
  vm = myproc()->vm;
  seg = newseg(vm);
  seg->base = vaddr;
  seg->end = vaddr + size;
  seg->r = 1; seg->w = 0; seg->x = 1;
}

void
vmstackinit(ulong vaddr, ulong size)
{
  segment *seg;
  pvm *vm;
  if (!PAGEALIGNED(vaddr) || !PAGEALIGNED(size))
    panic("align");
  vm = myproc()->vm;
  seg = newseg(vm);
  seg->base = vaddr;
  seg->end = vaddr + size;
  seg->r = 1; seg->w = 1; seg->x = 0;
  seg->stack = 1;
}

static pagetree*
treecopy(pagetree *pt)
{
  pagetree *npt;
  page *p;
  if (!pt)
    return NULL;
  npt = pageaddress(allocpage());
  for (int i = 0; i < 512; i++) {
    p = pt->pages[i];
    if (p)
      npt->pages[i] = getpage(p);
  }
  return npt;
}

static segment*
segdup(segment *seg)
{
  segment *nseg;
  if (!seg)
    return NULL;
  if (segreadonly(seg) || seg->share) {
    seg->ref++;
    return seg;
  } else if (seg->w) {
    nseg = allocseg();
    nseg->base = seg->base;
    nseg->end = seg->end;
    nseg->r = seg->r; nseg->w = seg->w; nseg->x = seg->x;
    if (seg->ino) {
      nseg->ino = idup(seg->ino);
      nseg->offset = seg->offset;
    }
    for (int i = 0; i < 64; i++)
      nseg->pt[i] = treecopy(seg->pt[i]);
    return nseg;
  } else {
    panic("w");
  }
  return NULL;
}

static void
invmap(void)
{
  pvm *vm = myproc()->vm;
  page *p, *tp;
  ulong flags;
  flags = irqsave();
  memset(vm->cr3, 0, PAGESIZE);
  memcpy(vm->cr3, kcr3, PAGESIZE);
  LIST_FOREACH_SAFE (p, tp, &vm->pgtp, e) {
    ldelete(&p->e);
    freepage(p);
  }
  vmswitch();
  irqrestore(flags);
}

pvm*
vmdup(void)
{
  pvm *old = myproc()->vm;
  pvm *vm;
  vm = procvm();
  if (!vm)
    return NULL;
  for (int i = 0; i < NSEG; i++)
    vm->seg[i] = segdup(old->seg[i]);
  invmap();
  return vm;
}

pvm*
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
  linit(&vm->pgtp);
  return vm;
failed:
  if (vm->cr3page)
    freepage(vm->cr3page);
  kfree(vm);
  return NULL;
}

void
vmswitch(void)
{
  proc *p = myproc();
  pvm *vm = p->vm;
  ulong flags;
  if (!vm)
    return;
  flags = irqsave();
  mycpu()->ts.rsp0 = (u64)pageaddress(p->kstackpage) + PAGESIZE;
  wrcr3(pa(vm->cr3));
  irqrestore(flags);
}
