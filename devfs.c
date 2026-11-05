#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <proc.h>
#include <fs.h>
#include <devfs.h>

static superblock *devsuper;
static devinode *devroot;
static LIST_HEAD(devlist);
static SPINLOCK(devllk);
static int nextdinum = 2;

static int
dinumalloc(void)
{
  return xadd(&nextdinum, 1);
}

static devinode*
getdevinode(superblock *sb, int inum)
{
  devinode *root = devroot;
  devinode *d;
  if (inum == 1)
    return root;
  lock(&devllk);
  LIST_FOREACH (d, &devlist, dent) {
    if(d->inum == inum) {
      unlock(&devllk);
      return d;
    }
  }
  unlock(&devllk);
  return NULL;
}

static inode*
devilookup(inode *pi, char *name)
{
  superblock *sb = pi->super;
  devinode *dpi = pi->data;
  devinode *d;
  inode *ino;
  LIST_FOREACH (d, &dpi->children, cent) {
    if (strcmp(d->name, name) == 0) {
      ino = iget(sb, d->inum);
      if (!ino)
        panic("devilookup");
      return ino;
    }
  }
  return NULL;
}

static inode*
devcreatei(inode *pi, char *name, u16 mode)
{
  superblock *sb = pi->super;
  devinode *dpi = pi->data;
  devinode *d;
  LIST_FOREACH (d, &dpi->children, cent) {
    if (strcmp(d->name, name) == 0)
      return NULL;
  }
  d = kmalloc(sizeof *d);
  d->mode = mode;
  strcpy(d->name, name);
  d->inum = dinumalloc();
  linit(&d->children);
  lappend(&dpi->children, &d->cent);
  lock(&devllk);
  lappend(&devlist, &d->dent);
  unlock(&devllk);
  return iget(sb, d->inum);
}

static int
devreadi(inode *ino, void *buf, uint off, uint n)
{
  int rc;
  devinode *di = ino->data;
  munlock(&ino->mtx);
  if (di->read)
    rc = di->read(di, buf, off, n);
  else
    rc = -1;
  mlock(&ino->mtx);
  return rc;
}

static int
devwritei(inode *ino, void *buf, uint off, uint n)
{
  int rc;
  devinode *di = ino->data;
  munlock(&ino->mtx);
  if (di->write)
    rc = di->write(di, buf, off, n);
  else
    rc = -1;
  mlock(&ino->mtx);
  return rc;
}

static int
devopen(inode *ino, file *f)
{
  return -1;
}

static int
devreaddir(inode *pi, dentry *dent)
{
  uint n = 0;
  superblock *sb = pi->super;
  devinode *dpi = pi->data;
  devinode *d;
  inode *ino;
  LIST_FOREACH (d, &dpi->children, cent) {
    if (dent->n == n) {
      dent->ino = iget(sb, d->inum);
      if (!dent->ino)
        panic("devilookup");
      memcpy(dent->name, d->name, sizeof(d->name));
      dent->namelen = strlen(d->name);
      return 1;
    }
    n++;
  }
  return 0;
}

static void
devinit(char *name, int (*init)(devinode*))
{
  inode *rooti = devsuper->rooti;
  inode *ino;
  devinode *di;
  mlock(&rooti->mtx);
  ino = createi(rooti, name, S_IRUSR|S_IWUSR);
  if (!ino)
    panic("omg");
  munlock(&rooti->mtx);
  di = ino->data;
  (*init)(di);
}

static int
deviget(inode *ino)
{
  superblock *sb = ino->super;
  devinode *di;
  di = getdevinode(sb, ino->inum);
  ino->mode = di->mode;
  ino->nlink = 1;
  ino->size = 0;
  ino->blocks = 0;
  ino->readi = devreadi;
  ino->writei = devwritei;
  ino->open = devopen;
  if (S_ISDIR(ino->mode)) {
    ino->readdir = devreaddir;
    ino->lookup = devilookup;
    ino->create = devcreatei;
  }
  ino->data = di;
  return 0;
}

static void
devsuperinit(superblock *sb)
{
  strcpy(sb->name, "devfs");
  sb->bsize = 0;
  sb->fssuper = NULL;
  sb->iget = deviget;
  sb->rooti = iget(sb, 1);
  if (!sb->rooti)
    panic("devsuper");
}

superblock*
devsuperblock(void)
{
  devsuper = kmalloc(sizeof *devsuper);
  if (!devsuper)
    panic("devroot");
  devsuper->dev = NULL;
  linit(&devsuper->itable);
  slockinit(&devsuper->lk);
  waitinit(&devsuper->wait);
  waitinit(&devsuper->freewait);
  devroot = kmalloc(sizeof *devroot);
  if (!devroot)
    panic("devroot");
  devroot->mode = S_IFDIR | S_IRUSR | S_IWUSR;
  devroot->inum = 1;
  linit(&devroot->children);
  devsuperinit(devsuper);
  devinit("cons", devconsinit);
  return devsuper;
}
