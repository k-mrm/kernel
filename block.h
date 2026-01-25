#ifndef _BLOCK_H
#define _BLOCK_H

#include <kernel.h>
#include <device.h>
#include <irq.h>

#define B_VALID         (1)
#define B_DIRTY         (1 << 1)
#define BSIZE		1024

struct buf {
	struct block *dev;

        int             bno;
        int             flags;
        unsigned char   data[BSIZE];
        uint            refcount;

	struct list bn;
};

struct block_if {
        int (*read)(struct block *dev, struct buf *b);
        int (*write)(struct block *dev, struct buf *b);
};

struct block {
        struct device dev;
	struct block_if *ops;
	struct list cache;
};

#define dev_block(_d)	container_of(_d, struct block, dev)

int probe_block(struct block *bdev);
struct buf *bread(struct block *dev, int bno);
struct buf *bootblock(struct block *dev);
struct buf *superblock(struct block *dev);
void brelease(struct buf *buf);
void bcachefree(void);
void bsync(void);

struct block *getblkdev(char *name);

#endif  // _BLOCK_H
