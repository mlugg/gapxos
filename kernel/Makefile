#
# MIT License
#
# Copyright (c) 2018 The gapxos Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

.POSIX:
.PHONY: all clean

LINKER := link.ld
LDFLAGS := -m64 -ffreestanding -T $(LINKER) -nostdlib -lgcc

CFLAGS := -Wall -Werror -m64 -z max-page-size=0x1000 -mno-red-zone -mno-mmx -mno-sse2 -c -g

# The below `ifeq` is a hack to fix compiling on the GCC toolchain installed on
# Gentoo systems.
ifeq ($(shell . /etc/os-release && echo $$NAME),Gentoo)
	CFLAGS := ${CFLAGS} -fno-stack-protector
endif

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
