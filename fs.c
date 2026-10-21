#include <kernel.h>
#include <sys.h>
#include <fs.h>
#include <proc.h>

static superblock *rootsuper;

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

static inode *
ialloc(superblock *sb, int inum)
{
  inode *ino;
  ino = kmalloc(sizeof *ino);
  if (!ino)
    return NULL;
  slockinit(&ino->lk);
  mtxinit(&ino->mtx);
  ino->super = sb;
  ino->inum = inum;
  ino->ref = 1;
  if (ext2iget(ino) < 0)
    goto fail;
  lock(&sb->lk);
  lappend(&sb->itable, &ino->e);
  unlock(&sb->lk);
  return ino;
fail:
  kfree(ino);
  return NULL;
}

void
isync(inode *ino)
{
  ;
}

inode *
iget(superblock *sb, int inum)
{
  inode *ino;
  lock(&sb->lk);
  LIST_FOREACH (ino, &sb->itable, e) {
    lock(&ino->lk);
    if (ino->inum == inum) {
      ino->ref++;
      unlock(&ino->lk);
      unlock(&sb->lk);
      return ino;
    }
    unlock(&ino->lk);
  }
  unlock(&sb->lk);
  return ialloc(sb, inum);
}

void
iput(inode *ino)
{
  superblock *sb = ino->super;
  lock(&ino->lk);
  if (ino->ref == 0)
    panic("iput");
  ino->ref--;
  if (ino->ref == 0) {
    // TODO: delete ino
    unlock(&ino->lk);
    lock(&sb->lk);
    ldelete(&ino->e);
    unlock(&sb->lk);
    kfree(ino);
  } else 
    unlock(&ino->lk);
}

inode *
idup(inode *ino)
{
  if (!ino)
    panic("idup");
  lock(&ino->lk);
  ino->ref++;
  unlock(&ino->lk);
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

// locked inode*
static inode*
traverse(inode *pi, char *path, char *name)
{
  int err;
  inode *ino;
  memset(name, 0, 256);
  while ((path = pathname(path, name, &err)) != NULL) {
    printk("tra n %s p=%s\n", name, path);
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
