#ifndef _SYSMEM_H
#define _SYSMEM_H

#include <kernel.h>

typedef struct MemBlock     MemBlock;
typedef struct MemChunk     MemChunk;
typedef struct Sysmem       Sysmem;

struct MemBlock
{
  Phys base;
  ulong size;
};

struct MemChunk
{
  char *name;
  uint nblock;
  MemBlock block[32];
};

struct Sysmem
{
  MemChunk avail;
  MemChunk rsrv;
  MemChunk allmem;
};

extern Sysmem sysmem;

#define FOREACH_MEMCHUNK_BLOCK(_chunk, _idx, _block)    \
  for (_block = (_chunk)->block, _idx = 0;        \
       _block < &(_chunk)->block[(_chunk)->nblock];       \
       _block++, _idx++)

#define FOREACH_SYSMEM_AVAIL_BLOCK(_block)      \
  for (_block = sysmem.avail.block;       \
       _block < &sysmem.avail.block[sysmem.avail.nblock]; \
       _block++)

#define FOREACH_SYSMEM_RSRV_BLOCK(_block)       \
  for (_block = sysmem.rsrv.block;        \
       _block < &sysmem.rsrv.block[sysmem.rsrv.nblock];   \
       _block++)

#define FOREACH_SYSMEM_ALL_BLOCK(_block)        \
  for (_block = sysmem.allmem.block;      \
       _block < &sysmem.allmem.block[sysmem.allmem.nblock];       \
       _block++)

static inline ulong
memstart (void)
{
  MemBlock *first = &sysmem.avail.block[0];
  return first->base;
}

static inline ulong
memend (void)
{
  MemBlock *last = &sysmem.avail.block[sysmem.avail.nblock-1];
  return last->base + last->size;
}

void sysavailmem (Phys base, u64 size);
void sysrsrvmem (Phys base, u64 size);
void *bootmemalloc (uint nbytes, uint align);

bool reservedaddr(ulong addr, ulong size);
void reservekernelarea (void);

#endif  // _SYSMEM_H
