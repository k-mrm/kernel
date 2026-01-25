#ifndef _KERNEL_H
#define _KERNEL_H

#define UNUSED          __attribute__ ((unused))
#define DEBUG           __attribute__ ((unused))
#define FALLTHROUGH     __attribute__ ((fallthrough))
#define PACKED          __attribute__ ((packed))
#define ALIGNED(n)      __attribute__ ((aligned(n)))
#define NORETURN        __attribute__ ((noreturn))
#define SECTION(s)      __attribute__ ((section(s)))
#define USED            __attribute__ ((used))

#define LIKELY(cond)    __builtin_expect (!!(cond), 1)
#define UNLIKELY(cond)  __builtin_expect (!!(cond), 0)

#define va_list         __builtin_va_list
#define va_start(v, l)  __builtin_va_start (v, l)
#define va_arg(v, l)    __builtin_va_arg (v, l)
#define va_end(v)       __builtin_va_end (v)
#define va_copy(d, s)   __builtin_va_copy (d, s)

typedef unsigned long   u64;
typedef long            i64;
typedef unsigned int    u32;
typedef signed int      i32;
typedef unsigned short  u16;
typedef signed short    i16;
typedef unsigned char   u8;
typedef signed char     i8;
typedef unsigned int    uint;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned char   uchar;

/* physical address */
typedef ulong   Phys;

#define NULL    ((void *)0)

typedef ulong   *PageTable;
typedef ulong   PTE;

typedef _Bool   bool;

#define true    1
#define false   0

#define offsetof(st, m)   ((ulong)((char *)&((st *)0)->m - (char *)0))

#define container_of(ptr, st, m)  \
  ({ const typeof(((st *)0)->m) *_mptr = (ptr); \
     (st *)((char *)_mptr - offsetof(st, m)); })

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define	KiB	(1024)
#define	MiB	(1024 * 1024)
#define	GiB	(1024 * 1024 * 1024)

#define USER

struct list
{
        struct list *prev, *next;
};

#define LIST_HEAD(_v)  struct list _v = {&(_v), &(_v)}

#define LIST_FOREACH(_pos, _head, _mem)        \
        for (_pos = container_of ((_head)->next, typeof (*_pos), _mem);      \
             &(_pos->_mem) != (_head);    \
             _pos = container_of (_pos->_mem.next, typeof (*_pos), _mem))

#define LIST_FOREACH_SAFE(_pos, _nv, _head, _mem)  \
        for (_pos = container_of ((_head)->next, typeof (*_pos), _mem);      \
             &(_pos->_mem) != (_head) && ((_nv = container_of(_pos->_mem.next, typeof(*_pos), _mem)), 1);    \
             _pos = _nv)

#define LIST_ENTRY(_h, _ty, _mem)       container_of((_h)->next, _ty, _mem)

static inline void
initlist(struct list *head)
{
        head->next = head;
        head->prev = head;
}

static inline void
list_add(struct list *h, struct list *e)
{
        e->next = h->next;
        h->next->prev = e;
        e->prev = h;
        h->next = e;
}

static inline int
list_len(struct list *h)
{
        struct list *e;
        int i = 0;

        for (e = h->next; e != h; e = e->next)
                i++;
        return i;
}

static inline bool
list_empty(struct list *h)
{
        return list_len(h) == 0;
}

static inline void
list_delete(struct list *e)
{
        struct list *n, *p;

        n = e->next;
        p = e->prev;
        p->next = n;
        n->prev = p;
        e->next = e->prev = NULL;
}

struct tree {
	struct tree *par;
	struct list child;
	struct list cn;
};

#define ROOT(_v)  struct tree _v = {NULL, {&((_v).child), &((_v).child)}, {0}}
#define TREE_ENTRY(_r, _ty, _mem)       container_of((_r), _ty, _mem)
#define PARENT(_t, _ty, _mem)		container_of((_t)->par, _ty, _mem)

static inline void
inittree(struct tree *t)
{
	t->par = NULL;
	initlist(&t->child);
}

static inline void
new_child(struct tree *t, struct tree *c)
{
	c->par = t;
	list_add(&t->child, &c->cn);
}

static inline void *
tree_dfs(struct tree *r, void *(*cb)(struct tree *, void *), void *arg)
{
	struct tree *t, *tmp;
	void *ret;

	LIST_FOREACH_SAFE (t, tmp, &r->child, cn) {
		ret = cb(t, arg);
		if (ret)
			return ret;
		tree_dfs(t, cb, arg);
	}

	return NULL;
}

static inline int
nchild(struct tree *t)
{
	return list_len(&t->child);
}

#endif	// _KERNEL_H
