#ifndef _VIRTIO_H
#define _VIRTIO_H

#include <types.h>
#include <compiler.h>
#include <pci.h>

typedef struct VirtioDevice   VirtioDevice;
typedef struct VirtioDeviceOp VirtioDeviceOp;
typedef struct VirtioDriver   VirtioDriver;
typedef struct VirtQ          VirtQ;

struct VirtioDeviceOp
{
        void    (*notify) (VirtioDevice *dev, int qsel);
        void    (*setvq) (VirtioDevice *dev, VirtQ *vq);
};

struct VirtioDriver
{
        char    name[16];

        uint    *features;
        uint    nfeatures;

        int     (*probe) (VirtioDevice *vdev);
        int     (*disconnect) (VirtioDevice *vdev);
};

struct VirtioDevice
{
        // for pci
        PciDev          *pci;
        // for mmio
        void            *iomem;
        Phys            iomemphys;

        VirtioDriver    *driver;

        VirtioDeviceOp  *op;
        void            *priv;
};

#endif  // _VIRTIO_H
