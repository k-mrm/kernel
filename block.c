#include <kernel.h>
#include <block.h>
#include <kalloc.h>
#include <panic.h>

static struct block *blockdev;

static struct buf *
balloc(struct block *dev)
{
	struct buf *buf;

        buf = zalloc();
        if (!buf)
                return NULL;

        buf->dev = dev;
        buf->refcount = 1;

	list_add(&dev->cache, &buf->bn);
        return buf;
}

static struct buf *
bget(struct block *dev, int bno)
{
        struct buf *buf;

	LIST_FOREACH (buf, &dev->cache, bn) {
                if (buf->bno == bno && buf->dev == dev) {
                        buf->refcount++;
                        return buf;
                }
	}

        buf = balloc(dev);
        buf->bno = bno;
        return buf;
}

struct buf *
bread (struct block *dev, int bno)
{
        struct buf *b = bget(dev, bno);
        if (!b)
                return NULL;

        if (!(b->flags & B_VALID)) {
                dev->ops->read(dev, b);
                b->flags |= B_VALID;
        }
        return b;
}

struct buf *
bootblock(struct block *dev)
{
        return bread(dev, 0);
}

struct buf *
superblock(struct block *dev)
{
        return bread(dev, 1);
}

void
brelease(struct buf *buf)
{
        if (buf->refcount == 0)
                panic ("refcount");

        buf->refcount--;

        if (buf->refcount == 0) {
                if (buf->flags & B_DIRTY)
                        buf->dev->ops->write(buf->dev, buf);
        }
}

void
bcachefree(void)
{
	struct block *dev = blockdev;
        struct buf *buf, *tmp;

        LIST_FOREACH_SAFE (buf, tmp, &dev->cache, bn) {
                if (buf->refcount == 0) {
                        list_delete(&buf->bn);
                        free(buf);
                }
        }
}

void
bsync(void)
{
	struct block *dev = blockdev;
        struct buf *buf;

        LIST_FOREACH (buf, &dev->cache, bn) {
                if (buf->refcount != 0 && buf->flags & B_DIRTY) {
                        buf->dev->ops->write(buf->dev, buf);
                        buf->flags &= ~B_DIRTY; 
                }
        }
}

int
probe_block(struct block *dev)
{
	initlist(&dev->cache);
        blockdev = dev;
        return 0;
}

struct block *
getblkdev(char *name)
{
        // TODO: name?
        if (!blockdev)
                panic ("no block device!");

        return blockdev;
}
