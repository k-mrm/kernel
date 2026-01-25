#ifndef _VIRTQUEUE_H
#define _VIRTQUEUE_H

#include <types.h>
#include <compiler.h>

#include "virtio.h"

typedef struct VQDesc     VQDesc;
typedef struct VQAvail    VQAvail;
typedef struct VQUsedElem VQUsedElem;
typedef struct VQUsed     VQUsed;
typedef struct VirtQ      VirtQ;

/* virtqueue */
#define NQUEUE  256

#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2 
#define VIRTQ_DESC_F_INDIRECT 4
struct VQDesc
{
        u64   addr;
        u32   len;
        u16   flags;
        u16   next;
} PACKED ALIGNED(16);

#define VIRTQ_AVAIL_F_NO_INTERRUPT  1
struct VQAvail
{
        u16   flags;
        u16   idx;
        u16   ring[NQUEUE];
} PACKED ALIGNED(16);

struct VQUsedElem
{
        u32   id;
        u32   len;
} PACKED;

#define VIRTQ_USED_F_NO_NOTIFY  1
struct VQUsed
{
        u16         flags;
        u16         idx;
        VQUsedElem  ring[NQUEUE];
} PACKED ALIGNED(4);

struct VirtQ
{
        VirtioDevice  *dev;

        // Lock     lock;
        VirtQ         *next;

        /* vring */
        VQDesc        *desc;
        VQAvail       *avail;
        VQUsed        *used;
        u16           num;

        u16           freehead;
        u16           nfree;
        u16           lastused;

        int           qsel;

        void          *cookie[NQUEUE];

        void          (*intrhandler)(struct virtq *);
};

typedef struct SBuf   SBuf;

// scattered buffer
struct SBuf
{
        void  *buf;
        u32   len;
};

void vqreg (VirtQ *vq);
void vqkick (VirtQ *vq);
int vqenqueue (VirtQ *vq, SBuf *s, int ns, void *cookie, bool in);
void *vqdequeue (VirtQ *vq, u32 *len);

VirtQ *newvirtq (VirtioDevice *dev, int qsel,
                void (*intrhandler) (VirtQ *));

static inline int
vqenqueuein (VirtQ *vq, SBuf *s, int ns, void *cookie)
{
        return vqenqueue (vq, s, ns, cookie, true);
}

static inline int
vqenqueueout (VirtQ *vq, SBuf *s, int ns, void *cookie)
{
        return vqenqueue (vq, s, ns, cookie, false);
}

#endif  // _VIRTQUEUE_H
