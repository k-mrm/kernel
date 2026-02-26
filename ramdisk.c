#include <kernel.h>
#include <device.h>
#include <block.h>
#include <string.h>

extern char _binary_fs_img_end[];
extern char _binary_fs_img_size[];
extern char _binary_fs_img_start[];

static struct ramdisk
{
  struct block block;

  void *base;
  int size;
} ramdisk;

static int
ramdisk_read(struct block *dev, struct buf *buf)
{
  struct ramdisk *disk = container_of(dev, struct ramdisk, block);
  int offset;

  offset = buf->bno * 1024;

  memcpy(buf->data, disk->base + offset, 1024);

  return 0;
}

static int
ramdisk_write(struct block *dev, struct buf *buf)
{
  struct ramdisk *disk = container_of(dev, struct ramdisk, block);
  int offset;

  offset = buf->bno * 1024;

  memcpy(disk->base + offset, buf->data, 1024);

  return 0;
}

static struct block_if ramdisk_if = {
  .read   = ramdisk_read,
  .write  = ramdisk_write,
};

static void
ramdisk_suspend(struct device *dev)
{
  ;
}

static void
ramdisk_resume(struct device *dev)
{
  ;
}

static int
ramdisk_probe(struct device *dev)
{
  struct block *bdev = dev_block(dev);
  struct ramdisk *rd = container_of(bdev, struct ramdisk, block);

  rd->base = _binary_fs_img_start;

  bdev->ops = &ramdisk_if;

  return probe_block(bdev);
}

static struct driver ramdisk_drv = {
  .name           = "RAMDisk",
  .description    = "RAMDisk Driver",
  .probe          = ramdisk_probe,
  .suspend        = ramdisk_suspend,
  .resume         = ramdisk_resume,
  .param          = "disable",
};

void
ramdiskinit (void)
{
  new_device(&ramdisk.block.dev, "block", "RAMDisk", &ramdisk_drv, NULL);
}
