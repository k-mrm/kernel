#include <kernel.h>
#include <sys.h>
#include <page.h>
#include <list.h>
#include <lock.h>

#define KOBJ_16   0
#define KOBJ_32   1
#define KOBJ_64   2
#define KOBJ_128  3
#define KOBJ_256  4
#define KOBJ_512  5
#define KOBJ_1024 6
#define NR_KOBJ   7

#define KOBJ_BSIZE(_m)  (1 << (((_m) - mlist) + 4))

typedef struct mfreelist mfreelist;

struct mfreelist {
  mfreelist *next;
};

struct mobj {
  spinlock lock;
  mfreelist *head;
  list pages; 
};

static mobj mlist[NR_KOBJ];

static inline int
ilog2(u64 x)
{
  int n = 0;
  while (x >>= 1)
    n++;
  return n;
}

static inline u64
up_pow2(u64 x)
{
  return (x <= 1) ? 1 : 1ULL << (ilog2(x - 1) + 1);
}

// locked p->obj
static void
__kfree(page *p, void *ptr)
{
  mfreelist *m;
  mobj *obj = p->obj;
  m = (mfreelist*)ptr;
  m->next = obj->head;
  obj->head = m;
  p->inuse++;
}

static void
kfreeobj(mobj *obj, page *p)
{
  void *ptr;
  uint bsize = KOBJ_BSIZE(obj);
  lappend(&obj->pages, &p->e);
  p->obj = obj;
  ptr = pageaddress(p);
  for (uint b = 0; b < PAGESIZE; b += bsize)
    __kfree(p, ptr + b);
}

void
kfree(void *ptr)
{
  page *p;
  if (!ptr)
    return;
  p = addresspage(ptr);
  if (!p)
    return;
  if (!p->obj)
    return;
  lock(&p->obj->lock);
  __kfree(p, ptr);
  unlock(&p->obj->lock);
}

void *
kmalloc(uint sz)
{
  int idx;
  mobj *obj;
  page *p;
  mfreelist *m;
  sz = up_pow2(sz);
  if (sz < 16)
    sz = 16;
  else if (sz > 1024)
    return NULL;
  idx = ilog2(sz) - 4;
  obj = mlist + idx;
  lock(&obj->lock);
  if (!obj->head) {
    p = allocpage();
    if (!p)
      return NULL;
    kfreeobj(obj, p);
  }
  m = obj->head;
  obj->head = m->next;
  p = addresspage((void*)m);
  p->inuse--;
  unlock(&obj->lock);
  return (void*)m;
}

void
kmallocinit(void)
{
  for (int i = 0; i < NR_KOBJ; i++) {
    slockinit(&mlist[i].lock);
    mlist[i].head = NULL;
    linit(&mlist[i].pages);
  }
}
