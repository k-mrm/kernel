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
OBJS += page.o

all: kernel.img

%.o: %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@echo AS $@
	@$(CC) $(CFLAGS) -c $< -o $@

bootblock: boot.o
	@echo LD $@
	@$(LD) $(LDFLAGS) -N -e start -Ttext 0x7c00 -o boot.elf $<
	@$(OBJCOPY) -S -O binary -j .text boot.elf $@
	@rm -f boot.elf

kernel.elf: $(OBJS) kernel.ld
	@echo LD $@
	@$(LD) -n $(LDFLAGS) -T kernel.ld -o $@ $(OBJS)

kernel.bin: kernel.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $< $@

kernel.img: bootblock kernel.bin
	@echo IMG $@
	@dd if=/dev/zero of=$@ bs=512 count=10000 status=none
	@dd if=bootblock of=$@ conv=notrunc bs=512 count=1 status=none
	@dd if=kernel.bin of=$@ conv=notrunc bs=512 seek=1 status=none

clean:
	@echo CLEAN
	$(RM) *.o *.d *.elf *.bin *.img bootblock

qemu: kernel.img
	$(QEMU) -nographic -drive file=kernel.img,index=0,media=disk,format=raw -smp $(NCPU) -m $(MEMSZ)

qemu-kvm: kernel.img
	$(QEMU) -accel kvm -cpu host -nographic -drive file=kernel.img,index=0,media=disk,format=raw -smp $(NCPU) -m $(MEMSZ)

.PHONY: all clean qemu qemu-kvm

-include $(OBJS:.o=.d) boot.d
