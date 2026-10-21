#include <kernel.h>
#include <sys.h>
#include <fs.h>
#include <lock.h>

typedef struct ext2dsuper ext2dsuper;
typedef struct ext2dinode ext2dinode;
typedef struct ext2dentry ext2dentry;
typedef struct ext2super ext2super;
typedef struct ext2inode ext2inode;
typedef struct ext2bgdesc ext2bgdesc;

/* inode.i_mode */
#define EXT2_S_IFSOCK 0xc000
#define EXT2_S_IFLNK  0xa000
#define EXT2_S_IFREG  0x8000
#define EXT2_S_IFBLK  0x6000
#define EXT2_S_IFDIR  0x4000
#define EXT2_S_IFCHR  0x2000
#define EXT2_S_IFIFO  0x1000

#define EXT2_S_ISUID  0x0800
#define EXT2_S_ISGID  0x0400
#define EXT2_S_ISVTX  0x0200

#define EXT2_S_IRUSR  0x0100
#define EXT2_S_IWUSR  0x0080
#define EXT2_S_IXUSR  0x0040
#define EXT2_S_IRGRP  0x0020
#define EXT2_S_IWGRP  0x0010
#define EXT2_S_IXGRP  0x0008
#define EXT2_S_IROTH  0x0004
#define EXT2_S_IWOTH  0x0002
#define EXT2_S_IXOTH  0x0001

/* inode number */
#define EXT2_BAD_INO            1
#define EXT2_ROOT_INO           2
#define EXT2_ACL_IDX_INO        3
#define EXT2_ACL_DATA_INO       4
#define EXT2_BOOT_LOADER_INO    5
#define EXT2_UNDEL_DIR_INO      6

/* ext2 filetype(ext2_dirent.file_type) */
#define EXT2_FT_UNKNOWN         0
#define EXT2_FT_REG_FILE        1
#define EXT2_FT_DIR             2
#define EXT2_FT_CHRDEV          3
#define EXT2_FT_BLKDEV          4
#define EXT2_FT_FIFO            5
#define EXT2_FT_SOCK            6
#define EXT2_FT_SYMLINK         7

struct ext2dsuper {
  u32 s_inodes_count;
  u32 s_blocks_count;
  u32 s_r_blocks_count;
  u32 s_free_blocks_count;
  u32 s_free_inodes_count;
  u32 s_first_data_block;
  u32 s_log_block_size;
  u32 s_log_frag_size;
  u32 s_blocks_per_group;
  u32 s_frags_per_group;
  u32 s_inodes_per_group;
  u32 s_mtime;
  u32 s_wtime;
  u16 s_mnt_count;
  u16 s_max_mnt_count;
  u16 s_magic;
  u16 s_state;
  u16 s_errors;
  u16 s_minor_rev_level;
  u32 s_lastcheck;
  u32 s_checkinterval;
  u32 s_creator_os;
  u32 s_rev_level;
  u16 s_def_resuid;
  u16 s_def_resgid;

  u32 s_first_ino;
  u16 s_inode_size;
  u16 s_block_group_nr;
  u32 s_feature_compat;
  u32 s_feature_incompat;
  u32 s_feature_ro_compat;
  u64 s_uuid[2];
  char s_volume_name[16];
  char s_last_mounted[64];
  u32 s_algo_bitmap;

  u8 s_prealloc_blocks;
  u8 s_prealloc_dir_blocks;
  u16 _align;

  u64 s_journal_uuid[2];
  u32 s_journal_inum;
  u32 s_journal_dev;
  u32 s_last_orphan;

  u32 s_hash_seed[4];
  u8 s_def_hash_version;
  u8 _padding[3];

  u32 s_default_mount_options;
  u32 s_first_meta_bg;
  char _unused[760];
} PACKED;

struct ext2bgdesc {
  u32 bg_block_bitmap;
  u32 bg_inode_bitmap;
  u32 bg_inode_table;
  u16 bg_free_blocks_count;
  u16 bg_free_inodes_count;
  u16 bg_used_dirs_count;
  u16 bg_pad;
  char bg_reserved[12];
} PACKED;

struct ext2dinode {
  u16 i_mode;
  u16 i_uid;
  u32 i_size;
  u32 i_atime;
  u32 i_ctime;
  u32 i_mtime;
  u32 i_dtime;
  u16 i_gid;
  u16 i_links_count;
  u32 i_blocks;
  u32 i_flags;
  u32 i_osd1;
  u32 i_block[15];
  u32 i_generation;
  u32 i_file_acl;
  u32 i_dir_acl;
  u32 i_faddr;
  u32 i_osd2[3];
} PACKED;

#define DT_UNKNOWN  0
#define DT_REG      1
#define DT_DIR      2
#define DT_CHR      3
#define DT_BLK      4
#define DT_FIFO     5
#define DT_SOCK     6
#define DT_LNK      7

struct ext2dentry {
  u32 inode;
  u16 rec_len;
  u8 name_len;
  u8 file_type;
  char name[];
} PACKED;
#define EXT2_DIRENT_NAME_MAX    255

struct ext2super {
  uint inosize;
  uint firstino;
  uint inocnt;
  uint blockcnt;
  uint bpg;
  uint ipg;
  uint ipb;
  uint ngroups;
  uint firstblock;
  ext2bgdesc **bgdesc;
};

struct ext2inode {
  u32 block[15];
};

static uint
bgstart(superblock *sb)
{
  return (1024 / sb->bsize) + 1;
}

void
ext2superinit(superblock *sb)
{
  ext2bgdesc *bg;
  ext2dsuper *ds;
  ext2super *es;
  uint groupblocks, dpb;
  block *b = sb->dev;
  ds = kmalloc(1024);
  if (!ds)
    panic("ds");
  __bread(b, 2, 2, ds);
  strcpy(sb->name, "ext2");
  es = kmalloc(sizeof *es);
  if (!es)
    panic("es");
  sb->bsize = 1024 << ds->s_log_block_size;
  es->inosize = ds->s_inode_size;
  es->firstino = ds->s_first_ino;
  es->inocnt = ds->s_inodes_count;
  es->blockcnt = ds->s_blocks_count;
  es->bpg = ds->s_blocks_per_group;
  es->ipg = ds->s_inodes_per_group;
  es->firstblock = ds->s_first_data_block;
  es->ngroups = (es->blockcnt - es->firstblock + es->bpg - 1) / es->bpg;
  dpb = sb->bsize / sizeof(ext2bgdesc);
  es->ipb = sb->bsize / es->inosize;
  groupblocks = (es->ngroups + dpb - 1) / dpb;
  es->bgdesc = kmalloc(sizeof(ext2bgdesc*) * groupblocks);
  for (int i = 0; i < groupblocks; i++) {
    bg = kmalloc(sb->bsize);
    if (!bg)
      panic("bg");
    bread(sb, bgstart(sb) + i, bg);
    es->bgdesc[i] = bg;
  }
  sb->fssuper = es;
  sb->rooti = iget(sb, EXT2_ROOT_INO);
  if (!sb->rooti)
    panic("ext2super");
  kfree(ds);
}

static int
blockpath(inode *ino, uint no, int *path)
{
  superblock *sb = ino->super;
  uint apb = sb->bsize / sizeof(u32);
  uint ndirect, nsingle, ndouble, ntriple;
  ndirect = 12;
  nsingle = apb;
  ndouble = apb*apb;
  ntriple = apb*apb*apb;
  if (no < ndirect) {
    *path++ = no;
    return 1;
  } else if ((no -= ndirect) < nsingle) {
    *path++ = 12;
    *path++ = no;
    return 2;
  } else if ((no -= nsingle) < ndouble) {
    *path++ = 13;
    *path++ = no / apb;
    *path++ = no % apb;
    return 3;
  } else if ((no -= ndouble) < ntriple) {
    *path++ = 14;
    *path++ = (no / apb) / apb;
    *path++ = (no / apb) % apb;
    *path++ = no % apb;
    return 4;
  } else {
    panic("oob");
  }
}

static uint
ext2iblock(inode *ino, uint no)
{
  superblock *sb = ino->super;
  ext2inode *ei = ino->data;
  int path[4];
  int depth;
  uint *blockptr, bno;
  void *tmp = kmalloc(sb->bsize);
  depth = blockpath(ino, no, path);
  blockptr = ei->block + path[0];
  for (int d = 1; d < depth; d++) {
    if (*blockptr == 0)
      panic("todo");
    bread(sb, *blockptr, tmp);
    blockptr = (uint*)tmp + path[d];
  }
  bno = *blockptr;
  kfree(tmp);
  return bno;
}

static int
ext2readi(inode *ino, void *buf, uint off, uint n)
{
  superblock *sb = ino->super;
  void *base = buf;
  void *tmp;
  u32 offblk, lastblk, offblkoff;
  uint bno;
  u64 cpsize;
  if (off > ino->size)
    return 0;
  if (off + n > ino->size)
    n = ino->size - off;
  offblk = off / sb->bsize;
  lastblk = (n + off) / sb->bsize;
  offblkoff = off % sb->bsize;
  for (uint i = offblk; i <= lastblk; i++) {
    tmp = kmalloc(sb->bsize);
    bno = ext2iblock(ino, i);
    if (bno)
      bread(sb, bno, tmp);
    cpsize = MIN(n, sb->bsize);
    if (offblkoff + cpsize > sb->bsize)
      cpsize = sb->bsize - offblkoff;
    memcpy(buf, tmp + offblkoff, cpsize);
    buf += cpsize;
    n -= cpsize;
    offblkoff = 0;
    kfree(tmp);
  }
  return buf - base;
}

static int
ext2writei(inode *ino, void *buf, uint off, uint n)
{
  superblock *sb = ino->super;
  void *base = buf;
  void *tmp;
  u32 offblk, lastblk, offblkoff;
  uint bno;
  u64 cpsize;
  if (off > ino->size)
    return 0;
  offblk = off / sb->bsize;
  lastblk = (n + off) / sb->bsize;
  offblkoff = off % sb->bsize;
  for (uint i = offblk; i <= lastblk; i++) {
    tmp = kmalloc(sb->bsize);
    bno = ext2iblock(ino, i);
    if (bno)
      bread(sb, bno, tmp);
    cpsize = MIN(n, sb->bsize);
    if (offblkoff + cpsize > sb->bsize)
      cpsize = sb->bsize - offblkoff;
    memcpy(buf, tmp + offblkoff, cpsize);
    buf += cpsize;
    n -= cpsize;
    offblkoff = 0;
    kfree(tmp);
  }
  return buf - base;
}

inode *
ext2lookup(inode *pi, char *name)
{
  superblock *sb = pi->super;
  ext2dentry *de;
  void *tmp;
  uint bno;
  inode *ino;
  uint reclen;
  if (strlen(name) > EXT2_DIRENT_NAME_MAX)
    return NULL;
  if (!S_ISDIR(pi->mode))
    return NULL;
  for (uint off = 0; off < pi->size; off += reclen) {
    tmp = kmalloc(sb->bsize);
    if (!tmp)
      panic("tmp");
    bno = ext2iblock(pi, off / sb->bsize);
    if (!bno) {
      kfree(tmp);
      return NULL;
    }
    bread(sb, bno, tmp);
    de = tmp + (off % sb->bsize);
    printk("traverse %s %d %d %d\n", de->name, de->inode, de->rec_len, pi->size);
    if (de->inode == 0 ||
        strlen(name) != de->name_len ||
        memcmp(name, de->name, de->name_len)) {
      reclen = de->rec_len;
      kfree(tmp);
      continue;
    }
    ino = iget(sb, de->inode);
    if (!ino)
      panic("lookup");
    kfree(tmp);
    return ino;
  }
  return NULL;
}

static ext2bgdesc *
inumbgdesc(superblock *sb, int inum)
{
  ext2super *es = sb->fssuper;
  u32 group, block, off, dpb;
  group = (inum - 1) / es->ipg;
  dpb = sb->bsize / sizeof(ext2bgdesc);
  block = group / dpb;
  off = group % dpb;
  return es->bgdesc[block] + off;
}

static ext2dinode *
rawinode(superblock *sb, int inum, void **bp)
{
  u32 block, off;
  u64 offbytes;
  void *buf;
  ext2bgdesc *bgrp;
  ext2super *es = sb->fssuper;
  *bp = NULL;
  if (!es)
    panic("es");
  if ((inum != EXT2_ROOT_INO && inum < es->firstino) ||
      inum > es->inocnt)
    return NULL;
  buf = kmalloc(sb->bsize);
  if (!buf)
    panic("rawinode");
  bgrp = inumbgdesc(sb, inum);
  offbytes = ((inum - 1) % es->ipg) * es->inosize;
  block = bgrp->bg_inode_table + offbytes / sb->bsize;
  off = offbytes % sb->bsize;
  bread(sb, block, buf);
  *bp = buf;
  return (ext2dinode*)(buf + off);
}

void
ext2sync(inode *ino)
{
}

int
ext2iget(inode *ino)
{
  ext2dinode *dino;
  ext2inode *ei;
  uint inum = ino->inum;
  superblock *sb = ino->super;
  void *buf;
  dino = rawinode(sb, inum, &buf);
  if (!dino)
    panic("ialloc");
  ei = kmalloc(sizeof *ei);
  if (!ei)
    return -1;
  ino->mode = dino->i_mode;
  ino->nlink = dino->i_links_count;
  ino->blocks = dino->i_blocks;
  ino->size = dino->i_size;
  memcpy(ei->block, dino->i_block, sizeof ei->block);
  ino->readi = ext2readi;
  ino->writei = ext2writei;
  ino->data = ei;
  kfree(buf);
  return 0;
}
