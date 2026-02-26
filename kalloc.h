#ifndef _KALLOC_H
#define _KALLOC_H

#include <kernel.h>
#include <vm.h>
#include <x86/mm.h>

typedef struct Page Page;
typedef struct PageBlock PageBlock;

struct Page {
  Page *next;
  u8 blockno;
};

struct PageBlock {
  Page *pages;
  Phys base;
  ulong npages;
  uint node; // NUMA Node
};

extern PageBlock proot[32];
extern uint nproot;

#define FOREACH_PAGEBLOCK(_pb) for (_pb = proot; _pb < &proot[nproot]; _pb++)

// FIXME: naive
static inline PageBlock *pa2block(Phys pa) {
  PageBlock *b;
  ulong size;

  FOREACH_PAGEBLOCK(b) {
    size = b->npages << PAGESHIFT;
    if (b->base <= pa && pa < b->base + size)
      return b;
  }
  return NULL;
}

static inline Page *pa2page(Phys pa) {
  PageBlock *block = pa2block(pa);
  return block->pages + ((pa - block->base) >> PAGESHIFT);
}

static inline Phys page2pa(Page *page) {
  PageBlock *block = &proot[page->blockno];
  return block->base + ((page - block->pages) << PAGESHIFT);
}

static inline Page *va2page(void *va) { return pa2page(V2P(va)); }

static inline void *page2va(Page *page) { return (void *)P2V(page2pa(page)); }

Page *allocpages(uint order);
void *alloczeropagesva(uint order);
void freepages(Page *pages, uint order);

#define zalloc() alloczeropagesva(0)
#define alloc() page2va(allocpages(0))
#define free(_addr) freepages(va2page(_addr), 0)

#define PA2PFN(_pa) ((_pa) >> PAGESHIFT)
#define PFN2PA(_pfn) ((_pfn) << PAGESHIFT)

void kallocinitearly(ulong start, ulong end);
void kallocinit(void);

#endif // _KALLOC_H
