#include <compiler.h>
#include <types.h>
#include <proc.h>
#include <block.h>
#include <module.h>
#include <pci.h>

#include "virtio.h"
#include "virtqueue.h"

#define KPREFIX         "virtio-blk:"

#include <printk.h>

typedef struct VirtioBlkCfg   VirtioBlkCfg;

#define VIRTIO_BLK_F_SIZE_MAX     (1)
#define VIRTIO_BLK_F_SEG_MAX      (2)
#define VIRTIO_BLK_F_GEOMETRY     (4)
#define VIRTIO_BLK_F_RO           (5)
#define VIRTIO_BLK_F_BLK_SIZE     (6)
#define VIRTIO_BLK_F_FLUSH        (9)
#define VIRTIO_BLK_F_TOPOLOGY     (10)
#define VIRTIO_BLK_F_CONFIG_WCE   (11)
#define VIRTIO_BLK_F_DISCARD      (13)
#define VIRTIO_BLK_F_WRITE_ZEROES (14)

struct VirtioBlkCfg
{
        u64     capacity;
        u32     sizemax;
        u32     segmax;
        struct VirtioBlkGeo
        {
                u16   cylinders;
                u8    heads;
                u8    sectors;
        } geometry;
        u32     blksize;
        struct VirtioBlkTopology
        {
                // # of logical blocks per physical block (log2)
                u8    physblockexp;
                // offset of first aligned logical block
                u8    alignoff;
                // suggested minimum I/O size in blocks
                u16   miniosize;
                // optimal (suggested maximum) I/O size in blocks
                u32   optiosize;
        } topology;
        u8      writeback;
        u8      unused0[3];
        u32     maxdiscardsect;
        u32     maxdiscardseg;
        u32     discardsectoralign;
        u32     maxwritezerosect;
        u32     maxwritezeroseg;
        u8      writezeroesmayunmap;
        u8      unused1[3];
} PACKED;

#define VIRTIO_BLK_S_OK        0
#define VIRTIO_BLK_S_IOERR     1
#define VIRTIO_BLK_S_UNSUPP    2
struct VirtioBlkReq
{
#define VIRTIO_BLK_T_IN           0
#define VIRTIO_BLK_T_OUT          1
#define VIRTIO_BLK_T_FLUSH        4
#define VIRTIO_BLK_T_DISCARD      11
#define VIRTIO_BLK_T_WRITE_ZEROES 13
        u32   type;
        u32   reserved;
        u64   sector;
        u8    data[];
} PACKED;

static int
virtblkpciprobe (PciDev *pci)
{
        log ("@%02x:%02x %04x:%04x\n", pci->bus, pci->devfn, pci->vendorid, pci->deviceid);

        return 0;
}

static PCI_ID virtblkid[] = {
        {0x1af4, 0x1001},
        {0x1af4, 0x1041},
        {0, 0},
};

static PciDriver virtioblkpcidrv = {
        .name           = "virtio-blk",
        .id             = virtblkid,
        .probe          = virtblkpciprobe,
};

void
virtblkinit (void)
{
        newpcidriver (&virtioblkpcidrv);
}

MODULE_DECL virtioblk = {
        .name           = "virtio-blk",
        .description    = "Virtio BLK Device Driver",
        .init           = virtblkinit,
        .delete         = NULL,
};
