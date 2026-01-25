CFLAGS := -Wall -Og -g -MD -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -I ../
LDFLAGS := -g -nostdlib -Wl,--build-id=none -static --no-relax

objs-1 += ../syscalls.o 
objs-1 += init.o
target = init

.SUFFIXES : .c .S .o

%.o: %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(target): $(objs-1)
	$(LD) -nostdlib -N -e main -Ttext 0x1000 -o $@ $(objs-1)

clean:
	$(RM) $(objs-1) $(target)
