#ifndef _MM_H
#define _MM_H

#include <kernel.h>

#define ALIGN(_p, align) (((ulong)(_p) + (align) - 1) & ~((align) - 1))
#define ALIGNDOWN(_p, align) ((ulong)(_p) & ~((align) - 1))
#define PAGEALIGNED(_p) (((ulong)(_p) & (PAGESIZE - 1)) == 0)
#define PAGEALIGN(_p) ALIGN((_p), PAGESIZE)
#define PAGEALIGNDOWN(_p) ALIGNDOWN((_p), PAGESIZE)
#define PAGEOFFSET(_p) ((ulong)(_p) & (PAGESIZE - 1))

struct vma {
  struct tree vn;

  ulong start;
  ulong end;
  ulong flags;
};

struct vm {
  PageTable pgdir;
  uint level;
  uint lowestlevel;
  bool user;
  struct proc *proc;
  void *ustack;
  u64 ustacksize;
  void *ustart;
  u64 csize;
  void *heap;
  u64 hsize;
  struct tree vma;
};

void kernelmap(void);
void *devmmap(ulong pa, ulong nbytes);
struct vm *kernelvm(void);
struct vm *allocvm(void);
void mappages(struct vm *vm, ulong va, ulong pa, ulong size, ulong flags,
              bool remap);
void copyvm(struct proc *proc, struct proc *np);
void copyin(struct vm *vm, ulong uva, void *buf, int size);
void vmcodealloc(struct vm *vm, ulong sz, ulong flags);
struct vm *uservm(struct proc *proc);
void freevm(struct vm *vm);

#define USTACKTOP 0x7ffffff000
#define USTACKBOTTOM (USTACKTOP - PAGESIZE)

#endif // __MM_H
