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
  ret = readi(ino, buf, f->pos, n);
  if (ret >= 0)
    f->pos += ret;
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
  ret = writei(ino, buf, f->pos, n);
  if (ret >= 0)
    f->pos += ret;
  munlock(&ino->mtx);
  return ret;
}

int
newpipe(int *fds)
{
  int fd1, fd2;
  file *f1, *f2;
  inode *ino1, *ino2;
  if (pipealloc(&ino1, &ino2) < 0)
    return -1;
  f1 = filealloc(ino1, O_READ);
  f2 = filealloc(ino2, O_WRITE);
  fd1 = fdalloc(f1);
  fd2 = fdalloc(f2);
  if (fd1 < 0 || fd2 < 0)
    return -1;
  fds[0] = fd1; fds[1] = fd2;
  return 0;
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

int
close(file *f)
{
  inode *ino;
  if (!f)
    return -1;
  if (--f->ref > 0)
    return 0;
  ino = f->ino;
  mlock(&ino->mtx);
  if (ino->close)
    ino->close(ino, f);
  munlock(&ino->mtx);
  iput(ino);
  kfree(f);
  return 0;
}
