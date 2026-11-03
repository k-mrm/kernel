#ifndef _DEVFS_H
#define _DEVFS_H

#include <kernel.h>
#include <list.h>
#include <sys.h>

struct devinode {
  u16 mode;
  char name[16];
  uint inum;
  list children;
  list cent;
  list dent;
  void *data;
  int (*read)(devinode*, void*, uint, uint);
  int (*write)(devinode*, void*, uint, uint);
};

#endif
