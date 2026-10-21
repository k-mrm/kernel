#include <kernel.h>
#include <sys.h>
#include <lock.h>
#include <block.h>
#include <list.h>

static LIST_HEAD(blocklist);
static int blocki = 0;
static char *bname[4] = {
  "hda", "hdb", "hdc", "hdd"
};
static LIST_HEAD(blru);
static SPINLOCK(lrulk);

block *
getblock(char *name)
{
  block *b;
  LIST_FOREACH (b, &blocklist, blist) {
    if (strcmp(name, b->name) == 0)
      return b;
  }
  return NULL;
}

int
__bread(block *b, ulong sect, int n, void *buf)
{
  return b->bio(b, 0, sect, n, buf);
}

int
__bwrite(block *b, ulong sect, int n, void *buf)
{
  return b->bio(b, 1, sect, n, buf);
}

block *
newblock(int (*bio)(block*, int, ulong, int, void*), void *data)
{
  block *b = kmalloc(sizeof *b);
  if (!b)
    return NULL;
  lappend(&blocklist, &b->blist);
  b->name = bname[blocki++];
  b->bio = bio;
  b->data = data;
  printk("%s disk\n", b->name);
  return b;
}
