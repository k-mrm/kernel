#ifndef _LIST_H
#define _LIST_H

#include <kernel.h>
#include <sys.h>

struct list {
  list *prev, *next;
};

#define LIST_HEAD(_v) list _v = {&(_v), &(_v)}

#define LIST_FOREACH(_pos, _head, _mem)                                        \
  for (_pos = container_of((_head)->next, typeof(*_pos), _mem);                \
       &(_pos->_mem) != (_head);                                               \
       _pos = container_of(_pos->_mem.next, typeof(*_pos), _mem))

#define LIST_FOREACH_SAFE(_pos, _nv, _head, _mem)                              \
  for (_pos = container_of((_head)->next, typeof(*_pos), _mem);                \
       &(_pos->_mem) != (_head) &&                                             \
       ((_nv = container_of(_pos->_mem.next, typeof(*_pos), _mem)), 1);        \
       _pos = _nv)

#define LIST_TOP(_h, _ty, _mem) container_of((_h)->next, _ty, _mem)

static inline void
linit(list *head)
{
  head->next = head;
  head->prev = head;
}

static inline void
linsert(list *n, list *prev, list *next)
{
  prev->next = n;
  n->next = next;
  n->prev = prev;
  next->prev = n;
}

static inline void
ladd(list *h, list *e)
{
  e->next = h->next;
  h->next->prev = e;
  e->prev = h;
  h->next = e;
}

static inline void
lappend(list *h, list *e)
{
  linsert(e, h->prev, h);
}

static inline int
llen(list *h)
{
  list *e;
  int i = 0;
  for (e = h->next; e != h; e = e->next)
    i++;
  return i;
}

static inline bool
lempty(list *h)
{
  return llen(h) == 0;
}

static inline void
ldelete(list *e)
{
  list *n, *p;
  n = e->next;
  p = e->prev;
  p->next = n;
  n->prev = p;
  e->next = e->prev = NULL;
}

#endif
