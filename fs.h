#ifndef _FS_H
#define _FS_H

#include <kernel.h>
#include <block.h>

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

struct superblock
{
        
};

struct dentry
{
        struct fs *fs;
	struct tree dn;
        char name[50];
	struct inode *inode;
};

struct inode
{
        struct fs *fs;
        void *priv;
        struct buf *buf;

	struct list in;

        u16     mode;
        u32     size;
        u32     atime;
        u32     ctime;
        u32     mtime;
        u32     dtime;
        u16     links_count;
        u32     blocks;
        u32     block[15];

        u32     inum;
        u32     major;
        u32     minor;
        u32     refcount;
        bool    new;
};

struct fs_if
{
        char *name;

	struct list fn;

        int (*probe)(struct fs *fs);
        struct inode *(*createi)(struct fs *fs, char *name, struct inode *dir, int mode, int dev);
        struct inode *(*iget)(struct fs *fs, int inum);
        int (*readi)(struct inode *ino, unsigned char *buf, u64 off, u64 size);
        int (*writei)(struct inode *ino, unsigned char *buf, u64 off, u64 size);
        int (*mkdir)(struct fs *fs, char *path);
        void (*sync)(struct inode *ino);
        int (*search)(struct inode *dir, char *basename);
        struct inode *(*rootinode)(struct fs *fs);
        bool (*fsisme)(unsigned char *sb);
};

struct fs
{
	struct block *dev;
	struct fs_if *op;

	struct superblock sb;

        void *priv;
	struct list itable;
};

void initfs(void);
void new_fs(struct fs_if *op);
void delete_fs(struct fs_if *op);
struct inode *idup(struct inode *ino);
struct inode *iget(struct fs *fs, int inum);
void iput(struct inode *ino);

struct inode *path2ino(const char *path);

#endif  // _FS_H
