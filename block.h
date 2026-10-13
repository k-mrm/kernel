#ifndef _BLOCK_H
#define _BLOCK_H

#include <kernel.h>
#include <sys.h>
#include <list.h>

struct block {
  char *name;
  void *data;
  uint secsize;
  int (*bio)(block*, int, ulong, void*);
  list blist;
};

#endif
