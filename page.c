#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <page.h>
#include <memlayout.h>
#include <list.h>
#include <lock.h>

struct {
  spinlock lk;
  list freelist;
} kmem;

pageblock pblock[32];
int nr_pblock = 0;

static int
kmem_npages(void)
{
  return llen(&kmem.freelist);
}

page *
allocpage(void)
{
  page *p = NULL;
  lock(&kmem.lk);
  if (!lempty(&kmem.freelist)) {
    p = LIST_TOP(&kmem.freelist, page, e);
    ldelete(&p->e);
    p->ref = 1;
  }
  unlock(&kmem.lk);
  if (p)
    memset(pageaddress(p), PAGESIZE, 0);
  return p;
}

void
freepage(page *p)
{
  if (!p)
    return;
  if (p->ref != 1)
    panic("p");
  p->ref = 0;
  lock(&kmem.lk);
  lappend(&kmem.freelist, &p->e);
  unlock(&kmem.lk);
}

page*
getpage(page *p)
{
  p->ref++;
  return p;
}

void
putpage(page *p)
{
  if (p->ref == 0)
    panic("put");
  p->ref--;
  if (p->ref == 0) {
    lock(&kmem.lk);
    lappend(&kmem.freelist, &p->e);
    unlock(&kmem.lk);
  }
}

static void
initblock(page *block, ulong base, ulong end)
{
  page *p;
  pageblock *pb;
  printk("initblock %p-%p\n", base, end);
  if (nr_pblock < 32) {
    pb = &pblock[nr_pblock++];
    pb->block = block;
    pb->base = base;
    pb->end = end;
  } else
    panic("nr_pblock >= 32");
  for (ulong c = base; c + PAGESIZE <= end; c += PAGESIZE) {
    p = block + ((c - (ulong)block) >> PAGESHIFT);
    p->block = block;
    p->ref = 1;
    freepage((void*)p);
  }
}

void
pageinit1(ulong end)
{
  u64 cbase, cend;
  uint npage = 0;
  page *block;
  slockinit(&kmem.lk);
  linit(&kmem.freelist);
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
  printk("kmem %d pages (%d KiB)\n", kmem_npages(), kmem_npages() << 2);
}
