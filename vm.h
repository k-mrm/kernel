#ifndef _VM_H
#define _VM_H

#include <kernel.h>
#include <sys.h>
#include <page.h>

struct segment {
  ulong base, end;
  list pagelist;
  bool r: 1;
  bool w: 1;
  bool x: 1;
  bool downgrow: 1;
};

struct pvm {
  ulong *cr3;
  page *cr3page;
  segment seg[3];
};

static inline segment *
stext(pvm *vm)
{
  return &vm->seg[0];
}

static inline segment *
sstack(pvm *vm)
{
  return &vm->seg[1];
}

#define USTACKTOP     0x7ffffff000

#endif
