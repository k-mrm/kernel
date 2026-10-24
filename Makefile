PREFIX =
CC = $(PREFIX)gcc
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

CFLAGS = -Wall -Og -g -MD -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -I.
LDFLAGS = -g -nostdlib -static --no-relax

QEMUPREFIX =
QEMU = $(QEMUPREFIX)qemu-system-x86_64

ifndef NCPU
NCPU = 1
endif

ifndef MEMSZ
MEMSZ = 512
endif

OBJS = entry.o main.o proc.o seg.o serial.o swtch.o trap.o traphandler.o printk.o
OBJS += page.o vm.o lib.o lapic.o kmalloc.o lock.o block.o ide.o ioapic.o wait.o
OBJS += syscall.o fs.o ext2.o

all: kernel.img user.img

%.o: %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@echo AS $@
	@$(CC) $(CFLAGS) -c $< -o $@

uinit: uinit.S
	$(CC) $(CFLAGS) -c uinit.S -o uinit.o
	$(LD) -nostdlib -N -e main -Ttext 0x1000 -o uinit.elf uinit.o
	$(OBJCOPY) -S -O binary uinit.elf uinit

bootblock: boot.o
	@echo LD $@
	@$(LD) $(LDFLAGS) -N -e start -Ttext 0x7c00 -o boot.elf $<
	@$(OBJCOPY) -S -O binary -j .text boot.elf $@
	@rm -f boot.elf

kernel.elf: $(OBJS) kernel.ld uinit
	@echo LD $@
	@$(LD) -n $(LDFLAGS) -T kernel.ld -o $@ $(OBJS) -b binary uinit

kernel.bin: kernel.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $< $@

USR_SRCS  := $(wildcard usr/*.c)
USR_PROGS := $(USR_SRCS:usr/%.c=rootfs/%)
USR_CFLAGS = -Wall -Og -g -ffreestanding -nostdinc -nostdlib -nostartfiles -Iusr

rootfs/%: usr/%.c usr/syscall.S usr/user.ld usr/user.h
	@echo CC $@
	@$(CC) $(USR_CFLAGS) -c $< -o $(<:.c=.o)
	@$(CC) $(USR_CFLAGS) -c usr/syscall.S -o usr/syscall.o
	@$(LD) -nostdlib -T usr/user.ld -o $@ $(<:.c=.o) usr/syscall.o
	@rm -f $(<:.c=.o) usr/syscall.o

user.img: $(USR_PROGS) $(shell find rootfs -type f)
	@echo IMG $@
	@dd if=/dev/zero of=$@ bs=1k count=1440 status=none
	@mkfs.ext2 -d rootfs $@

kernel.img: bootblock kernel.bin
	@echo IMG $@
	@dd if=/dev/zero of=$@ bs=512 count=10000 status=none
	@dd if=bootblock of=$@ conv=notrunc bs=512 count=1 status=none
	@dd if=kernel.bin of=$@ conv=notrunc bs=512 seek=1 status=none

clean:
	@echo CLEAN
	$(RM) *.o *.d *.elf *.bin *.img bootblock .gdbinit

QEMUOPTS = -nographic -drive file=kernel.img,index=0,media=disk,format=raw -drive file=user.img,index=1,media=disk,format=raw -smp $(NCPU) -m $(MEMSZ)

qemu: kernel.img user.img
	$(QEMU) $(QEMUOPTS)

qemu-kvm: kernel.img
	$(QEMU) -accel kvm -cpu host $(QEMUOPTS)

qemu-gdb: kernel.img .gdbinit
	@echo "Waiting for gdb on tcp::1234 ..."
	$(QEMU) $(QEMUOPTS) -s -S

gdb: kernel.elf .gdbinit
	gdb kernel.elf

.gdbinit:
	@echo "set architecture i386:x86-64"          >  $@
	@echo "set disassembly-flavor att"             >> $@
	@echo "target remote localhost:1234"           >> $@
	@echo "symbol-file kernel.elf"                 >> $@
	@echo "set confirm off"                        >> $@

.PHONY: all clean qemu qemu-kvm qemu-gdb gdb

-include $(OBJS:.o=.d) boot.d
