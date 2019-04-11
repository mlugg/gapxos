.POSIX:
.PHONY: all clean

LINKER := link.ld
LDFLAGS := -m32 -ffreestanding -T $(LINKER) -nostdlib -lgcc -no-pie -fno-pic -O0

CFLAGS := -Wall -Werror -m32 -no-pie -fno-pic -c -g -O0 -fno-stack-protector

SOURCE := $(shell find . -type f -name '*.c')
OBJECTS := $(patsubst %.c,%.o,$(SOURCE))

ASM_SOURCE := $(shell find . -type f -name '*.s')
ASM_OBJECTS := $(patsubst %.s,%.o,$(ASM_SOURCE))

all: loader

clean:
	rm -f loader
	rm -f *.o
	rm -f loader.sym

loader: $(OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	objcopy --only-keep-debug $@ $@.sym
	objcopy --strip-debug $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.s
	$(CC) $(CFLAGS) -o $@ $^
