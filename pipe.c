#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <proc.h>
#include <fs.h>
#include <file.h>
#include <page.h>
#include <lock.h>

typedef struct pipe pipe;
typedef struct pipeinode pipeinode;

struct pipe {
  spinlock lk;
  waitpoint pwait;
  ulong prod, cons;
  waitpoint rwait;
  waitpoint wwait;
  page *bufpage;
  void *buf;
  int plock;
  int wantrd;
  int wantwr;
  int readers, writers;
};

struct pipeinode {
  u16 mode;
  char name[16];
  uint inum;
  list children;
  list cent;
  void *data;
  pipe *pipe;
};

static superblock *pipesuper;
static pipeinode *piperoot;
static int nextpinum = 2;

static int
pinumalloc(void)
{
  return xadd(&nextpinum, 1);
}

static pipeinode*
getpipeinode(superblock *sb, int inum)
{
  pipeinode *root = piperoot;
  pipeinode *p;
  if (inum == 1)
    return root;
  LIST_FOREACH (p, &root->children, cent) {
    if(p->inum == inum)
      return p;
  }
  return NULL;
}

static inode*
pipeilookup(inode *pi, char *name)
{
  superblock *sb = pi->super;
  pipeinode *ppi = pi->data;
  pipeinode *p;
  inode *ino;
  LIST_FOREACH (p, &ppi->children, cent) {
    if (strcmp(p->name, name) == 0) {
      ino = iget(sb, p->inum);
      if (!ino)
        panic("pipeilookup");
      return ino;
    }
  }
  return NULL;
}

static inode*
pipecreatei(inode *pi, char *name, u16 mode)
{
  superblock *sb = pi->super;
  pipeinode *ppi = pi->data;
  pipeinode *p;
  p = kmalloc(sizeof *p);
  p->mode = mode;
  strcpy(p->name, name);
  p->inum = pinumalloc();
  linit(&p->children);
  lappend(&ppi->children, &p->cent);
  return iget(sb, p->inum);
}

// locked p
static void
pipelock(pipe *p)
{
  while (p->plock)
    sleep(&p->pwait, &p->lk, false);
  p->plock = 1;
}

// locked p
static void
pipeunlock(pipe *p)
{
  p->plock = 0;
  wakeup(&p->pwait);
}

// locked p
static bool
pipeempty(pipe *p)
{
  return p->prod == p->cons;
}

// locked p
static bool
pipefull(pipe *p)
{
  return p->cons + PAGESIZE == p->prod;
}

// locked p
static ulong
pipecnt(pipe *p)
{
  return p->prod - p->cons;
}

// locked p
static ulong
piperemain(pipe *p)
{
  return PAGESIZE - pipecnt(p);
}

static int
pread(pipe *pipe, void *buf, uint n)
{
  ulong size;
  uint c = 0;
  lock(&pipe->lk);
  pipelock(pipe);
  while (n) {
    if (!pipeempty(pipe)) {
      size = PAGESIZE - pipe->cons % PAGESIZE;
      if (size > pipecnt(pipe))
        size = pipecnt(pipe);
      if (size > n)
        size = n;
      unlock(&pipe->lk);
      memcpy(buf + c, pipe->buf + (pipe->cons % PAGESIZE), size);
      lock(&pipe->lk);
      pipe->cons += size;
      c += size;
      n -= size;
    } else if (!pipe->writers) {
      break;
    } else {
      if (c)
        break;
      if (pipe->wantwr) {
        wakeup(&pipe->wwait);
        pipe->wantwr = 0;
      }
      pipeunlock(pipe);
      pipe->wantrd = 1;
      if (sleep(&pipe->rwait, &pipe->lk, true) < 0) {
        unlock(&pipe->lk);
        return -1;
      }
      pipelock(pipe);
    }
  }
  pipeunlock(pipe);
  if (pipe->wantwr) {
    wakeup(&pipe->wwait);
    pipe->wantwr = 0;
  }
  unlock(&pipe->lk);
  return c;
}

static int
pwrite(pipe *pipe, void *buf, uint n)
{
  ulong size;
  uint c = 0;
  lock(&pipe->lk);
  pipelock(pipe);
  while (n) {
    if (!pipefull(pipe) && pipe->readers) {
      size = PAGESIZE - pipe->prod % PAGESIZE;
      if (size > piperemain(pipe))
        size = piperemain(pipe);
      if (size > n)
        size = n;
      unlock(&pipe->lk);
      memcpy(pipe->buf + (pipe->prod % PAGESIZE), buf + c, size);
      lock(&pipe->lk);
      pipe->prod += size;
      c += size;
      n -= size;
    } else {
      if (!pipe->readers)
        break;
      if (c)
        break;
      if (pipe->wantrd) {
        wakeup(&pipe->rwait);
        pipe->wantrd = 0;
      }
      pipeunlock(pipe);
      pipe->wantwr = 1;
      if (sleep(&pipe->wwait, &pipe->lk, true) < 0) {
        unlock(&pipe->lk);
        return -1;
      }
      pipelock(pipe);
    }
  }
  pipeunlock(pipe);
  if (pipe->wantrd) {
    wakeup(&pipe->rwait);
    pipe->wantrd = 0;
  }
  unlock(&pipe->lk);
  return c;
}

static int
pipereadi(inode *ino, void *buf, uint off, uint n)
{
  int rc;
  pipeinode *pi = ino->data;
  munlock(&ino->mtx);
  rc = pread(pi->pipe, buf, n);
  mlock(&ino->mtx);
  return rc;
}

static int
pipewritei(inode *ino, void *buf, uint off, uint n)
{
  int rc;
  pipeinode *pi = ino->data;
  munlock(&ino->mtx);
  rc = pwrite(pi->pipe, buf, n);
  mlock(&ino->mtx);
  return rc;
}

static int
pipereaddir(inode *pi, dentry *dent)
{
  uint n = 0;
  superblock *sb = pi->super;
  pipeinode *ppi = pi->data;
  pipeinode *p;
  LIST_FOREACH (p, &ppi->children, cent) {
    if (dent->n == n) {
      dent->ino = iget(sb, p->inum);
      if (!dent->ino)
        panic("pipeilookup");
      memcpy(dent->name, p->name, sizeof(p->name));
      dent->namelen = strlen(p->name);
      return 1;
    }
    n++;
  }
  return 0;
}

static void
freepipe(pipe *p)
{
  freepage(p->bufpage);
  kfree(p);
}

static int
pipeclose(inode *ino, file *f)
{
  pipeinode *pi = ino->data;
  pipe *pipe = pi->pipe;
  lock(&pipe->lk);
  pipelock(pipe);
  if (ino->mode & S_IRUSR)
    pipe->readers--;
  else if (ino->mode & S_IWUSR)
    pipe->writers--;
  else
    panic("pipeclose");
  if (!pipe->readers != !pipe->writers) {
    if (pipe->wantwr) {
      wakeup(&pipe->wwait);
      pipe->wantwr = 0;
    }
    if (pipe->wantrd) {
      wakeup(&pipe->rwait);
      pipe->wantrd = 0;
    }
  }
  pipeunlock(pipe);
  unlock(&pipe->lk);
  if (pipe->readers == 0 && pipe->writers == 0)
    freepipe(pipe);
  return 0;
}

int
pipealloc(inode **ip1, inode **ip2)
{
  inode *rooti = pipesuper->rooti;
  inode *ino1, *ino2;
  pipeinode *p1, *p2;
  pipe *pipe;
  mlock(&rooti->mtx);
  ino1 = createi(rooti, "pipe-r", S_IRUSR);
  ino2 = createi(rooti, "pipe-w", S_IWUSR);
  munlock(&rooti->mtx);
  if (!ino1 || !ino2)
    return -1;
  pipe = kmalloc(sizeof *pipe);
  if (!pipe)
    return -1;
  pipe->bufpage = allocpage();
  if (!pipe->bufpage)
    return -1;
  pipe->buf = pageaddress(pipe->bufpage);
  waitinit(&pipe->pwait);
  waitinit(&pipe->rwait);
  waitinit(&pipe->wwait);
  slockinit(&pipe->lk);
  pipe->readers = pipe->writers = 1;
  p1 = ino1->data; p2 = ino2->data;
  p1->pipe = p2->pipe = pipe;
  *ip1 = ino1; *ip2 = ino2;
  return 0;
}

static int
pipeiget(inode *ino)
{
  superblock *sb = ino->super;
  pipeinode *pi;
  pi = getpipeinode(sb, ino->inum);
  ino->mode = pi->mode;
  ino->nlink = 1;
  ino->size = 0;
  ino->blocks = 0;
  if (S_ISDIR(ino->mode)) {
    ino->readdir = pipereaddir;
    ino->lookup = pipeilookup;
    ino->create = pipecreatei;
  } else {
    ino->readi = pipereadi;
    ino->writei = pipewritei;
    ino->close = pipeclose;
  }
  ino->data = pi;
  return 0;
}

static void
pipesuperinit(superblock *sb)
{
  strcpy(sb->name, "pipe");
  sb->bsize = 0;
  sb->fssuper = NULL;
  sb->iget = pipeiget;
  sb->rooti = iget(sb, 1);
  if (!sb->rooti)
    panic("pipesuper");
}

superblock*
pipesuperblock(void)
{
  pipesuper = kmalloc(sizeof *pipesuper);
  if (!pipesuper)
    panic("piperoot");
  pipesuper->dev = NULL;
  linit(&pipesuper->itable);
  slockinit(&pipesuper->lk);
  waitinit(&pipesuper->wait);
  waitinit(&pipesuper->freewait);
  piperoot = kmalloc(sizeof *piperoot);
  if (!piperoot)
    panic("piperoot");
  piperoot->mode = S_IFDIR | S_IRUSR | S_IWUSR;
  piperoot->inum = 1;
  linit(&piperoot->children);
  pipesuperinit(pipesuper);
  return pipesuper;
}
