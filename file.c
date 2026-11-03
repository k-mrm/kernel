#include <kernel.h>
#include <sys.h>
#include <fs.h>
#include <proc.h>
#include <file.h>
#include <lock.h>

static int
fdalloc(file *f)
{
  proc *p = myproc();
  for (int i = 0; i < 16; i++) {
    if (!p->openf[i]) {
      p->openf[i] = f;
      return i;
    }
  }
  return -1;
}

static file*
filealloc(inode *ino, int mode)
{
  file *f;
  f = kmalloc(sizeof *f);
  if (!f)
    return NULL;
  f->ino = ino;
  f->ref = 1;
  f->pos = 0;
  switch (mode & 3) {
    case O_READ:
    case O_EXEC:
      f->r = 1; f->w = 0;
      break;
    case O_WRITE:
      f->r = 0; f->w = 1;
      break;
    case O_RDWR:
      f->r = 1; f->w = 1;
      break;
  }
  return f;
}

file*
fdup(file *f)
{
  f->ref++;
  return f;
}

int
read(file *f, void *buf, uint n)
{
  inode *ino;
  int ret;
  if (!f->r)
    return -1;
  ino = f->ino;
  mlock(&ino->mtx);
  if (ino->readi) {
    ret = ino->readi(ino, buf, f->pos, n);
    if (ret >= 0)
      f->pos += ret;
  } else {
    ret = -1;
  }
  munlock(&ino->mtx);
  return ret;
}

int
write(file *f, void *buf, uint n)
{
  inode *ino;
  int ret;
  if (!f->w)
    return -1;
  ino = f->ino;
  mlock(&ino->mtx);
  if (ino->writei) {
    ret = ino->writei(ino, buf, f->pos, n);
    if (ret >= 0)
      f->pos += ret;
  } else {
    ret = -1;
  }
  munlock(&ino->mtx);
  return ret;
}

int
open(char *path, int mode)
{
  file *f;
  inode *ino;
  int fd;
  if (mode & O_CREATE) {
    return -1;
  } else {
    ino = namei(path, mode);
    if (!ino)
      return -1;
  }
  mlock(&ino->mtx);
  f = filealloc(ino, mode);
  if (!f) {
    munlock(&ino->mtx);
    return -1;
  }
  fd = fdalloc(f);
  if (fd < 0) {
    kfree(f);
    munlock(&ino->mtx);
    return -1;
  }
  if (ino->open)
    ino->open(ino, f);
  munlock(&ino->mtx);
  return fd;
}
