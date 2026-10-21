#ifndef _FS_H
#define _FS_H

#include <kernel.h>
#include <sys.h>
#include <fs.h>
#include <list.h>
#include <block.h>
#include <lock.h>

#define S_IFMT    0xf000

#define S_IFSOCK  0xc000
#define S_IFLNK   0xa000
#define S_IFREG   0x8000
#define S_IFBLK   0x6000
#define S_IFDIR   0x4000
#define S_IFCHR   0x2000
#define S_IFIFO   0x1000

#define S_ISUID   0x0800
#define S_ISGID   0x0400
#define S_ISVTX   0x0200

#define S_IRUSR   0x0100
#define S_IWUSR   0x0080
#define S_IXUSR   0x0040
#define S_IRGRP   0x0020
#define S_IWGRP   0x0010
#define S_IXGRP   0x0008
#define S_IROTH   0x0004
#define S_IWOTH   0x0002
#define S_IXOTH   0x0001

#define S_ISSOCK(_m)    (((_m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(_m)     (((_m) & S_IFMT) == S_IFLNK)
#define S_ISREG(_m)     (((_m) & S_IFMT) == S_IFREG)
#define S_ISBLK(_m)     (((_m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(_m)     (((_m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(_m)     (((_m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(_m)    (((_m) & S_IFMT) == S_IFIFO)

struct superblock {
  block *dev;
  char name[16];
  list itable;
  spinlock lk;
  inode *rooti;
  void *fssuper;
  uint bsize;
};

struct inode {
  spinlock lk;
  superblock *super;
  mutex mtx;
  list e;
  u16 mode;
  u16 nlink;
  u32 blocks;
  u32 size;
  int inum;
  int ref;
  void *data;
  int (*readi)(inode*, void*, uint, uint);
  int (*writei)(inode*, void*, uint, uint);
};

#endif  
