#include <kernel.h>
#include <sys.h>
#include <fs.h>
#include <proc.h>

static superblock *rootsuper;

static void __iput(inode*);

int
bread(superblock *sb, ulong bno, void *buf)
{
  ulong nsect = sb->bsize / 512;
  ulong sect = nsect * bno;
  return __bread(sb->dev, sect, nsect, buf);
}

int
bwrite(superblock *sb, ulong bno, void *buf)
{
  ulong nsect = sb->bsize / 512;
  ulong sect = nsect * bno;
  return __bwrite(sb->dev, sect, nsect, buf);
}

void
isync(inode *ino)
{
  ;
}

// locked sb
static inode*
findi(superblock *sb, int inum)
{
  inode *ino;
retry:
  LIST_FOREACH (ino, &sb->itable, e) {
    if (ino->inum == inum) {
      if (ino->free) {
        sleep(&sb->freewait, &sb->lk);
        goto retry;
      }
      if (ino->ref == 0 || ino->err)
        panic("findi");
      ino->ref++;
      return ino;
    }
  }
  return NULL;
}

inode *
iget(superblock *sb, int inum)
{
  inode *ino;
  int err = 0;
  lock(&sb->lk);
retry:
  ino = findi(sb, inum);
  if (ino) {
    while (ino->new)
      sleep(&sb->wait, &sb->lk);
    if (ino->err) {
      __iput(ino);
      goto retry;
    }
    unlock(&sb->lk);
    return ino;
  }
  ino = kmalloc(sizeof *ino);
  if (!ino) {
    unlock(&sb->lk);
    return NULL;
  }
  mtxinit(&ino->mtx);
  ino->super = sb;
  ino->inum = inum;
  ino->ref = 1;
  ino->new = 1;
  ino->free = 0;
  lappend(&sb->itable, &ino->e);
  unlock(&sb->lk);
  if (ext2iget(ino) < 0)
    err = 1;
  lock(&sb->lk);
  ino->new = 0;
  ino->err = err;
  if (err) {
    ldelete(&ino->e);
    __iput(ino);
  }
  wakeup(&sb->wait);
  unlock(&sb->lk);
  return err ? NULL : ino;
}

static void
__iput(inode *ino)
{
  superblock *sb = ino->super;
  int err = ino->err;
  if (ino->ref == 0)
    panic("iput");
  if (--ino->ref > 0)
    return;
  ino->free = 1;
  unlock(&sb->lk);
  if (!err) {
    // TODO: free and update ino
    ;
  }
  lock(&sb->lk);
  if (!err)
    ldelete(&ino->e);
  kfree(ino);
  wakeup(&sb->freewait);
}

void
iput(inode *ino)
{
  superblock *sb = ino->super;
  lock(&sb->lk);
  __iput(ino);
  unlock(&sb->lk);
}

inode *
idup(inode *ino)
{
  superblock *sb;
  if (!ino)
    panic("idup");
  sb = ino->super;
  lock(&sb->lk);
  ino->ref++;
  unlock(&sb->lk);
  return ino;
}

void
mountroot(void)
{
  block *rootimg;
  rootimg = getblock("hdb");
  if (!rootimg)
    panic("root");
  rootsuper = kmalloc(sizeof *rootsuper);
  if (!rootsuper)
    panic("root");
  rootsuper->dev = rootimg;
  linit(&rootsuper->itable);
  slockinit(&rootsuper->lk);
  waitinit(&rootsuper->wait);
  waitinit(&rootsuper->freewait);
  ext2superinit(rootsuper);
}

// must hold pi->mtx
static inode*
lookup(inode *pi, char *name)
{
  return ext2lookup(pi, name);
}

// must hold ino->mtx
int
readi(inode *ino, void *buf, uint off, uint n)
{
  return ino->readi(ino, buf, off, n);
}

// must hold ino->mtx
int
writei(inode *ino, void *buf, uint off, uint n)
{
  return ino->writei(ino, buf, off, n);
}

static char*
pathname(char *path, char *name, int *err)
{
  int len = 0;
  /* skip '/' ("////aaa///bbb/ccc" -> "aaa///bbb/ccc") */
  while (*path == '/')
    path++;
  if (*path == '\0')
    return NULL;
  /* get elem and inc path (get "aaa" from "aaa///bbb/ccc", path = "///bbb/ccc") */
  while (*path != '\0' && *path != '/') {
    *name++ = *path++;
    if (++len > 256) {
      *err = 1;
      return NULL;
    }
  }
  *err = 0;
  return path;
}

static inode*
traverse(inode *pi, char *path, char *name)
{
  int err;
  inode *ino;
  memset(name, 0, 256);
  while ((path = pathname(path, name, &err)) != NULL) {
    mlock(&pi->mtx);
    if (!S_ISDIR(pi->mode))
      goto fail;
    if (err)
      goto fail;
    ino = lookup(pi, name);
    if (!ino)
      goto fail;
    munlock(&pi->mtx);
    iput(pi);
    pi = ino;
  }
  return pi;
fail:
  munlock(&pi->mtx);
  iput(pi);
  return NULL;
}

inode*
path2inode(char *path)
{
  proc *p;
  inode *ino;
  char name[256];
  if (!rootsuper)
    panic("super");
  if (*path == '/') {
    ino = idup(rootsuper->rooti);
  } else {
    p = myproc();
    if (!p)
      panic("path2inode");
    ino = idup(p->cwd);
  }
  return traverse(ino, path, name);
}

inode*
namei(char *path, int mode)
{
  inode *ino;
  ino = path2inode(path);
  if (!ino)
    return NULL;
  switch (mode & 3) {
    case O_READ:
      if (ino->mode & S_IRUSR)
        return ino;
      break;
    case O_WRITE:
      if (ino->mode & S_IWUSR)
        return ino;
      break;
    case O_RDWR:
      if ((ino->mode & (S_IWUSR|S_IRUSR)) == (S_IWUSR|S_IRUSR))
        return ino;
      break;
    case O_EXEC:
      if ((ino->mode & (S_IXUSR|S_IRUSR)) == (S_IXUSR|S_IRUSR))
        return ino;
      break;
    default: panic("namei");
  }
  return NULL;
}
