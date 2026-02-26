#include <fs.h>
#include <kalloc.h>
#include <kernel.h>
#include <panic.h>
#include <proc.h>
#include <string.h>
#include <syscall.h>

#define KPREFIX "fs:"

#include <printk.h>

static LIST_HEAD(filesystem);
static struct fs *rootfs;

void new_fs(struct fs_if *op) {
  list_add(&filesystem, &op->fn);
  trace("New Filesystem: %s\n", op->name);
}

void delete_fs(struct fs_if *op) { list_delete(&op->fn); }

static struct fs_if *identfs(struct block *dev) {
  struct buf *sb;
  struct fs_if *fsif;

  sb = superblock(dev);
  if (!sb)
    return NULL;

  LIST_FOREACH(fsif, &filesystem, fn) {
    if (fsif->fsisme(sb->data))
      goto found;
  }

  fsif = NULL;

found:
  brelease(sb);
  return fsif;
}

static struct inode *ialloc(struct fs *fs, int inum) {
  struct inode *ino;
  ino = zalloc();
  ino->fs = fs;
  ino->inum = inum;
  ino->refcount = 1;
  ino->new = true;
  list_add(&fs->itable, &ino->in);
  return ino;
}

struct inode *iget(struct fs *fs, int inum) {
  struct inode *ino;

  LIST_FOREACH(ino, &fs->itable, in) {
    if (ino->fs == fs && ino->inum == inum) {
      ino->refcount++;
      return ino;
    }
  }
  return ialloc(fs, inum);
}

void iput(struct inode *ino) {
  if (ino->refcount == 0)
    panic("iput");
  ino->refcount--;
  if (ino->refcount == 0) {
    brelease(ino->buf);
    list_delete(&ino->in);
    free(ino);
  }
}

struct inode *idup(struct inode *ino) {
  ino->refcount++;
  return ino;
}

static const char *skippath(const char *path, char *name, int *err) {
  int len = 0;

  /* skip '/' ("////aaa/bbb" -> "aaa/bbb") */
  while (*path == '/')
    path++;
  /* get elem and inc path (get "aaa/" from "aaa/bbb", path = "bbb") */
  while ((*name = *path) && *path++ != '/') {
    name++;

    if (++len > 128) {
      *err = 1;
      return NULL;
    }
  }
  /* cut '/' from name ("aaa/" -> "aaa") */
  if (*name == '/')
    *name = 0;
  return path;
}

static struct inode *traverse(struct fs *fs, struct inode *pi, const char *path,
                              char *name) {
  int err = 0, inum;

  path = skippath(path, name, &err);
  if (err)
    goto err;
  if (*path == 0 && *name == 0)
    goto ret;
  if (!S_ISDIR(pi->mode))
    goto ret;
  inum = fs->op->search(pi, name);
  if (inum < 0)
    return NULL;

  iput(pi);
  pi = fs->op->iget(fs, inum);
  memset(name, 0, 128);
  return traverse(fs, pi, path, name);
ret:
  return pi;
err:
  iput(pi);
  return NULL;
}

struct inode *path2ino(const char *path) {
  struct cpu *cpu;
  struct proc *cp;
  struct inode *ino;
  struct fs *fs;
  char name[128] = {0};

  if (*path == '/') {
    fs = rootfs;
    ino = fs->op->rootinode(fs);
  } else {
    cpu = mycpu();
    cp = cpu->current;
    ino = idup(cp->cwd);
    fs = ino->fs;
  }
  return traverse(fs, ino, path, name);
}

static void regmountpoint(char *path, struct fs *fs) {
  if (strcmp(path, "/") == 0)
    rootfs = fs;
  else
    panic("unimpl");
}

int mountfs(char *path, char *name) {
  struct block *dev;
  struct fs_if *fsif;
  struct fs *fs;
  int ret;

  dev = getblkdev(name);
  if (!dev)
    return -1;
  fsif = identfs(dev);
  if (!fsif)
    return -1;
  fs = zalloc();
  if (!fs)
    return -1;
  fs->dev = dev;
  fs->op = fsif;
  initlist(&fs->itable);
  if (!fs->op->probe)
    return -1;
  ret = fs->op->probe(fs);
  regmountpoint(path, fs);
  return ret;
}

static int mountroot(void) { return mountfs("/", "sda"); }

void fsdbg(void) {
  struct inode *ino = path2ino("/README.md");
  if (ino) {
    struct fs *fs = ino->fs;
    unsigned char buf[128] = {0};
    trace("found inum: %d\n", ino->inum);
    fs->op->readi(ino, buf, 0, 127);
    trace("%s\n", buf);
    // iput (ino);
  } else {
    trace("not found\n");
  }
}

// stub
static int write(int fd, const char *USER buf, unsigned long size) {
  int n = 0;
  if (fd == 1) {
    for (n = 0; n < size; n++)
      printk("%c", buf[n]);
  }
  return n;
}

SYSCALL_DEFINE(SYS_WRITE, write);

static int read(int fd, char *buf, unsigned long size) {
  int n = 0;
  if (fd == 0)
    n = consread(buf, size);
  return n;
}

SYSCALL_DEFINE(SYS_READ, read);

void initfs(void) {
  if (list_empty(&filesystem))
    goto nofs;
  if (mountroot() < 0)
    goto nofs;
  fsdbg();
  return;
nofs:
  panic("No FileSystem");
}
