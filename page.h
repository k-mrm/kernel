#ifndef _PAGE_H
#define _PAGE_H

#include <asm.h>
#include <sys.h>

#define PAGESIZE  0x1000
#define PAGESHIFT 12

#define PTE_PA_MASK 0xfffffffff000ull

struct page {
  page *block;
  page *next;
  int ref;
}; 

#define ALIGN(_p, align)	(((ulong)(_p) + (align)-1) & ~((align)-1))
#define ALIGNDOWN(_p, align)	((ulong)(_p) & ~((align)-1))
#define PAGEALIGNED(_p)		(((ulong)(_p) & (PAGESIZE-1)) == 0)
#define PAGEALIGN(_p)		ALIGN((_p), PAGESIZE)
#define PAGEALIGNDOWN(_p)	ALIGNDOWN((_p), PAGESIZE)
#define PAGEOFFSET(_p)		((ulong)(_p) & (PAGESIZE-1))

void *
pageaddress(page *p)
{
  int pageno = p - p->block;
  return (void*)p->block + pageno * PAGESIZE;
}

#endif
