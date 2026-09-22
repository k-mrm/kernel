#ifndef _PAGE_H
#define _PAGE_H

#include <asm.h>
#include <sys.h>
#include <list.h>

#define PAGESIZE  0x1000
#define PAGESHIFT 12

#define PTE_PA_MASK 0xfffffffff000ull

typedef struct pageblock pageblock;

struct pageblock {
  ulong base, end;
  page *block;
};

extern pageblock pblock[32];
extern int nr_pblock;

struct page {
  page *block;
  mobj *obj;  // for kmalloc.c
  list e;
  uint inuse; // for kmalloc.c
  uint ref;
}; 

#define ALIGN(_p, align)	(((ulong)(_p) + (align)-1) & ~((align)-1))
#define ALIGNDOWN(_p, align)	((ulong)(_p) & ~((align)-1))
#define PAGEALIGNED(_p)		(((ulong)(_p) & (PAGESIZE-1)) == 0)
#define PAGEALIGN(_p)		ALIGN((_p), PAGESIZE)
#define PAGEALIGNDOWN(_p)	ALIGNDOWN((_p), PAGESIZE)
#define PAGEOFFSET(_p)		((ulong)(_p) & (PAGESIZE-1))

static inline void *
pageaddress(page *p)
{
  int pageno = p - p->block;
  return (void*)p->block + pageno * PAGESIZE;
}

static inline page *
addresspage(void *ptr)
{
  pageblock *pb;
  ulong p = (ulong)PAGEALIGNDOWN(ptr);
  uint pageno;
  for (int i = 0; i < nr_pblock; i++) {
    pb = pblock + i;
    if (pb->base <= p && p < pb->end)
      goto found;
  }
  return NULL;
found:
  pageno = (p - (ulong)pb->block) >> PAGESHIFT;
  return pb->block + pageno;
}

#endif
