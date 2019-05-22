.POSIX:
.PHONY: all clean

LINKER := link.ld
LDFLAGS := -m64 -ffreestanding -T $(LINKER) -nostdlib -lgcc -pie

CFLAGS := -Wall -Werror -m64 -z max-page-size=0x1000 -mno-red-zone -mno-mmx -mno-sse2 -c -g -fno-stack-protector -fPIE -fPIC

SOURCE := $(shell find . -type f -name '*.c')
OBJECTS := $(patsubst %.c,%.o,$(SOURCE))

ASM_SOURCE := $(shell find . -type f -name '*.s')
ASM_OBJECTS := $(patsubst %.s,%.o,$(ASM_SOURCE))

all: kernel

clean:
	rm -f kernel
	rm -f $(shell find . -type f -name '*.o')
	rm -f kernel.sym

kernel: $(OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	objcopy --only-keep-debug $@ $@.sym
	objcopy --strip-debug $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.s
	$(CC) $(CFLAGS) -o $@ $^
