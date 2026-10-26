#ifndef _VM_H
#define _VM_H

#include <kernel.h>
#include <sys.h>
#include <page.h>
#include <lock.h>

struct pagetree {
  page *pages[512];
};

struct segment {
  ulong base, end;
  pagetree *pt[64]; // 2MiB * 64 = 128 MiB
  spinlock lk;
  inode *ino;
  uint offset;
  uint ref;
  uint share;
  bool r: 1;
  bool w: 1;
  bool x: 1;
  bool stack: 1;
};

#define NSEG  6

struct pvm {
  ulong *cr3;
  page *cr3page;
  list pgtp;
  segment *seg[NSEG];
};

#define USTACKTOP     0x7ffffff000

#define VM_READ   1
#define VM_WRITE  2
#define VM_EXEC   4

#endif
