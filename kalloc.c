// Physical Memory Buddy Allocator for Kernel

#include <kernel.h>
#include <string.h>
#include <kalloc.h>
#include <sysmem.h>
#include <vm.h>
#include <panic.h>

#define KPREFIX   "kalloc:"

#include <printk.h>

#define MAX_ORDER   9

typedef struct FreeChunk    FreeChunk;
typedef struct AllocBlock   AllocBlock;

PageBlock proot[32];
uint      nproot = 0;

static ulong earlystart, earlyend;

struct FreeChunk
{
        Page *freelist;
        uint nfree;
};

struct AllocBlock
{
        // SPINLOCK;
        FreeChunk chunk[MAX_ORDER+1];
};

static AllocBlock kblock;

static PageBlock *
newpageblock (void)
{
        if (nproot < 32)
                return &proot[nproot++];
        else
                return NULL;
}

static void DEBUG
kallocdump (void)
{
        FreeChunk *c;
        ulong     nbytes = 0;

        for (uint order = 0; order < MAX_ORDER + 1; order++)
        {
                c = kblock.chunk + order;
                trace ("%d bytes page(order%d): %d Pages\n", PAGESIZE * (1 << order), order, c->nfree);
                nbytes += PAGESIZE * (1 << order) * c->nfree;
        }

        trace ("Total: %d Bytes\n", nbytes);
}

static Page *
chunkdeletepa (FreeChunk *chunk, Phys pa)
{
        Page *prev = NULL;

        for (Page *p = chunk->freelist; p; prev = p, p = p->next)
        {
                if (page2pa (p) == pa)
                {
                        chunk->nfree--;
                        if (prev)
                                prev->next = p->next;
                        else
                                chunk->freelist = p->next;
                        return p;
                }
        }
        return NULL;
}

static void
chunkaddpage (FreeChunk *chunk, Page *page)
{
        page->next = chunk->freelist;
        chunk->freelist = page;
        chunk->nfree++;
}

static void
splitpage (AllocBlock *kb, Page *p, uint order, uint blockorder)
{
        FreeChunk   *c;
        Page        *latter;
        uint        pagesize;

        while (blockorder > order)
        {
                blockorder--;
                c = kb->chunk + blockorder;
                pagesize = 1 << blockorder;
                latter = p + pagesize;
                chunkaddpage (c, latter);
        }
}

static Page *
__allocpages (AllocBlock *kb, uint order)
{
        FreeChunk *c;
        Page      *p;

        for (uint i = order; i < MAX_ORDER; i++)
        {
                c = kb->chunk + i;
                if (!c->freelist)
                {
                        // empty
                        continue;
                }
                p = c->freelist;
                c->freelist = p->next;
                c->nfree--;
                splitpage (kb, p, order, i);
                return p;
        }
        return NULL;
}

Page *
allocpages (uint order)
{
        return __allocpages (&kblock, order);
}

void *
alloczeropagesva (uint order)
{
	Page *page;
	void *va;

	page = allocpages(order);

	if (!page)
		return NULL;

        va = page2va(page);
	memset(va, 0, 1 << order << PAGESHIFT);
	return va;
}

#define BUDDY(pfn, order)       ((pfn) ^ (1 << (order)))

static void
mergepage (AllocBlock *kb, Page *page, uint order)
{
        FreeChunk *chunk;
        ulong     pfn, buddypfn;
        Page      *buddy;
        Page      *p0;

        if (order > MAX_ORDER)
                return;

        chunk = kb->chunk + order;
        pfn = PA2PFN (page2pa (page));
        buddypfn = BUDDY (pfn, order);

        // Lock chunk
        if (order != MAX_ORDER &&
            (buddy = chunkdeletepa (chunk, PFN2PA (buddypfn))) != NULL)
        {
                // mergeable page
                p0 = pfn < buddypfn ? page : buddy;
                mergepage (kb, p0, order + 1);
        }
        else
        {
                chunkaddpage (chunk, page);
        }
        // Unlock chunk
}

void
freepages (Page *page, uint order)
{
        mergepage (&kblock, page, order);
}

static void
initpageblock (MemBlock *block)
{
        PageBlock *pb;
        Phys      bend = PAGEALIGNDOWN (block->base + block->size);

        pb = newpageblock ();
        if (!pb)
                panic ("null page block");

        pb->base    = PAGEALIGN (block->base);
        pb->npages  = (bend - pb->base) >> PAGESHIFT;
        pb->pages   = bootmemalloc (pb->npages * sizeof(Page), sizeof(Page));
        pb->node    = 0;

        if (!pb->pages)
                panic ("initpageblock failed\n");
}

#define PBLOCKNO(pb)      ((pb) - proot)

static uint
earlyfreeblock (PageBlock *pb)
{
	Page *page;
	Phys addr     = pb->base;
	uint bno      = PBLOCKNO (pb);
	ulong npages  = 0;

	trace("early free block: %p-%p %d bytes\n", pb->base, pb->base + (pb->npages << PAGESHIFT),
			pb->npages << PAGESHIFT);

	for (ulong i = 0; i < pb->npages; i++, addr += PAGESIZE) {
		page = pb->pages + i;
		page->blockno = bno;
		// FIXME: naive
		if (!reservedaddr(addr, PAGESIZE)) {
			npages++;
			freepages (page, 0);
		}
	}
	return npages;
}

void
kallocinitearly (ulong start, ulong end)
{
        MemBlock  *block;
        PageBlock *pblock;
        ulong     npages = 0;

        trace ("initialize %p-%p\n", start, end);
        memset(&kblock, 0, sizeof kblock);

        earlystart  = start;
        earlyend    = end;

        FOREACH_SYSMEM_AVAIL_BLOCK (block) {
                initpageblock (block);
        }

        FOREACH_PAGEBLOCK (pblock) {
                npages += earlyfreeblock (pblock);
        }

        kallocdump ();
        if (npages == 0)
                panic ("system has no memory!");
}

void
kallocinit (void)
{
        ;
}
