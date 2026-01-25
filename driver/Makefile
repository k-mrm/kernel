PREFIX = 
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

CFLAGS := -Wall -Og -g -MD -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -I ../
LDFLAGS := -g -nostdlib -nostartfiles -Wl,--build-id=none -static --no-relax

.SUFFIXES : .c .S .o

test.o: test.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o *.d
