.POSIX:
.PHONY: all clean

LINKER := link.ld
LDFLAGS := -m64 -ffreestanding -T $(LINKER) -nostdlib -lgcc

CFLAGS := -Wall -Werror -m64 -z max-page-size=0x1000 -mno-red-zone -mno-mmx -mno-sse2 -c -g

SOURCE := $(wildcard *.c) $(wildcard **/*.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCE))

ASM_SOURCE := $(wildcard *.s) $(wildcard **/*.s)
ASM_OBJECTS := $(patsubst %.s,%.o,$(ASM_SOURCE))

all: kernel

clean:
	rm -f kernel
	rm -f *.o
	rm -f **/*.o
	rm -f kernel.sym

kernel: $(OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	objcopy --only-keep-debug $@ $@.sym
	objcopy --strip-debug $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.s
	$(CC) $(CFLAGS) -o $@ $^
