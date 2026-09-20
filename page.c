#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <page.h>
#include <memlayout.h>

struct {
  // spinlock lock;
  page *freelist;
} kmem;

static int
kmem_npages(void)
{
  page *p;
  uint n = 0;
  for (p = kmem.freelist; p != NULL; p = p->next)
    n++;
  return n;
}

page *
kalloc(void)
{
  page *p;
  p = kmem.freelist;
  if (p) {
    kmem.freelist = p->next;
    p->ref = 1;
    memset(p, PAGESIZE, 0);
  }
  return p;
}

void
kfree(page *p)
{
  if (!p)
    return;
  if (!p->ref)
    return;
  p->ref--;
  if (!p->ref) {
    p->next = kmem.freelist;
    kmem.freelist = p;
  }
}

static void
initblock(page *block, ulong base, ulong end)
{
  page *p;
  printk("initblock %p-%p\n", base, end);
  for (ulong c = base; c + PAGESIZE <= end; c += PAGESIZE) {
    p = block + ((c - (ulong)block) >> PAGESHIFT);
    p->block = block;
    p->ref = 1;
    kfree((void*)p);
  }
}

void
pageinit1(ulong end)
{
  u64 cbase, cend;
  uint npage = 0;
  page *block;
  for (int i = 0; i < e820count; i++) {
    printk("e820 memory %p-%p %d\n", e820[i].base, e820[i].base+e820[i].len, e820[i].type);
    if (e820[i].type != 1)
      continue;
    cbase = (ulong)va(e820[i].base);
    cend = (ulong)va(e820[i].base + e820[i].len);
    if (cend < (ulong)va(0x100000))
      continue;
    cbase = MAX(cbase, (ulong)va(0x100000));
    end = MIN(end, cend);
    cbase = PAGEALIGN(cbase);
    block = (page*)cbase;
    npage = (cend - cbase) >> PAGESHIFT;
    cbase += npage * sizeof (page);
    initblock(block, PAGEALIGN(cbase), end);
  }
  
  printk("kmem %d pages (%d KiB) %d\n", kmem_npages(), kmem_npages() << 2, npage);
}
