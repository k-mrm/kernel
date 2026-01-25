QEMUPREFIX ?=
QEMU = $(QEMUPREFIX)qemu-system-x86_64

PREFIX = 
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

CFLAGS := -Wall -Og -g -MD -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -I ./
LDFLAGS := -g -nostdlib -Wl,--build-id=none -static --no-relax

NAME = kernel
elf = $(NAME).elf
img = $(NAME).img
iso = $(NAME).iso
map = $(NAME).map

NCPU ?= 1
MEMSZ ?= 512

ARCH ?= x86

# Kernel objects (arch-specific)
objs-x86 += x86/entry.o
objs-x86 += x86/com.o x86/main.o x86/seg.o
objs-x86 += x86/cpu.o x86/pic-8259a.o x86/mm.o
objs-x86 += x86/trap-handler.o x86/trap.o
objs-x86 += x86/apic.o x86/xapic.o x86/x2apic.o
objs-x86 += x86/cswitch.o x86/pci.o

# Kernel objects
objs-1 += printk.o string.o main.o console.o
objs-1 += hpet.o sysmem.o panic.o
objs-1 += multiboot.o acpi.o device.o timer.o
objs-1 += vm.o kalloc.o irq.o proc.o cpu.o
objs-1 += module.o block.o fs.o ramdisk.o syscall.o

# Kernel Modules
objs-1 += driver/pci.o # driver/test.o driver/virtio-blk.o driver/virtio.o driver/virtqueue.o
objs-1 += fs/ext2.o

QEMUOPTS = -smp $(NCPU) -m $(MEMSZ) -no-reboot
QEMUOPTS += -device virtio-net-pci,bus=pci.0,disable-legacy=on,disable-modern=off
QEMUOPTS += -device virtio-rng-pci,bus=pci.0,disable-legacy=on,disable-modern=off

.SUFFIXES : .c .S .o

%.o: %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

fs.img:
	dd if=/dev/zero of=fs.img count=10000
	mkfs -t ext2 -d rootfs/ -v fs.img -b 1024

initcode: $(ARCH)/initcode.S
	$(CC) $(CFLAGS) -c $(ARCH)/initcode.S -o initcode.o
	$(LD) -nostdlib -N -e main -Ttext 0x1000 -o initcode.elf initcode.o
	$(OBJCOPY) -S -O binary initcode.elf initcode

$(elf): $(ARCH)/link.ld $(objs-1) $(objs-$(ARCH)) $(CONFIG) fs.img initcode
	@echo LD $@
	@$(LD) -n -Map $(map) --no-relax -T $(ARCH)/link.ld -o $@ $(objs-1) $(objs-$(ARCH)) -b binary fs.img initcode

elf: $(elf)
	@make $(elf)

iso: $(iso)

$(iso): elf grub.cfg
	@mkdir -p iso/boot/grub
	@cp grub.cfg iso/boot/grub
	@cp $(elf) iso/boot/
	@grub-mkrescue -o $@ iso/

clean:
	$(RM) $(objs-1) $(objs-$(ARCH)) $(elf) $(iso) $(img) $(map) fs.img initcode
	$(RM) -rf iso/

#qemu-img: $(img)
#	$(QEMU) -nographic -drive file=$(img),index=0,media=disk,format=raw -smp $(NCPU) -m $(MEMSZ)

qemu-iso: $(iso)
	$(QEMU) $(QEMUOPTS) -nographic -machine type=pc -drive file=$(iso),format=raw -serial mon:stdio

qemu-gdb: $(iso)
	$(QEMU) $(QEMUOPTS) -nographic -drive file=$(iso),format=raw -serial mon:stdio -S -gdb tcp::1234

.PHONY: clean elf iso qemu-iso qemu-gdb
