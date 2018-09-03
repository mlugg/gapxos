.POSIX:
.PHONY: all clean

LINKER := link.ld
LDFLAGS := -m32 -ffreestanding -T $(LINKER) -nostdlib -lgcc -no-pie -fno-pic -O0

CFLAGS := -Wall -Werror -m32 -no-pie -fno-pic -c -g -O0

# The below `ifeq` is a hack to fix compiling on the GCC toolchain installed on
# Gentoo systems.
ifeq ($(shell . /etc/os-release && echo $$NAME),Gentoo)
	CFLAGS := ${CFLAGS} -fno-stack-protector
endif

SOURCE := $(wildcard *.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCE))

ASM_SOURCE := $(wildcard *.s)
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
