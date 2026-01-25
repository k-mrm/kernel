#include <types.h>
#include <compiler.h>
#include <kalloc.h>
#include <printk.h>

#include "virtqueue.h"

void
vqreg (VirtQ *vq)
{
        VirtioDevice *dev = vq->dev;

        dev->op->setvq (dev, vq);
}

void
vqkick (VirtQ *vq)
{
        VirtioDevice *dev = vq->dev;

        if (!(vq->used->flags & VIRTQ_USED_F_NO_NOTIFY))
                dev->op->notify (dev, vq->qsel);
}

int
vqenqueue (VirtQ *vq, SBuf *s, int ns, void *cookie, bool in)
{
        return -1;
}

void *
vqdequeue (VirtQ *vq, u32 *len)
{
        return NULL;
}

VirtQ *
newvirtq (VirtioDevice *dev, int qsel, void (*intrhandler) (VirtQ *))
{
        VirtQ *vq = zalloc ();
        int i;

        if (!vq)
                panic ("vq");

        vq->desc  = zalloc ();
        vq->avail = zalloc ();
        vq->used  = zalloc ();

        log ("virtq d %p a %p u %p\n", vq->desc, vq->avail, vq->used);
        for (i = 0; i < NQUEUE - 1; i++)
                vq->desc[i].next = i + 1;
        vq->desc[i].next = 0xffff;    /* last entry */

        vq->dev         = dev;
        vq->qsel        = qsel;
        vq->num         = vq->nfree = NQUEUE;
        vq->freehead    = 0;
        vq->lastused    = 0;
        vq->intrhandler = intrhandler;
        // initlock vq->lock
        return vq;
}
