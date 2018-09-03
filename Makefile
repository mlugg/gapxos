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
.PHONY: all clean iso test

all:
	+$(MAKE) -C loader
	+$(MAKE) -C kernel

clean:
	+$(MAKE) clean -C loader
	+$(MAKE) clean -C kernel
	rm -rf ./iso
	rm -rf ./os.iso

iso: all
	mkdir -p ./iso/boot/grub
	cp ./kernel/kernel ./iso/boot/kernel.bin
	cp ./loader/loader ./iso/boot/loader.bin
	cp ./bootloader/grub.cfg ./iso/boot/grub/grub.cfg
	grub-mkrescue -o ./os.iso ./iso
	rm -rf ./iso

test: iso
	qemu-system-x86_64 -boot d -cdrom os.iso -m 64 -s -d guest_errors &
	sleep 2s
	gdb -x ./dbg/gdb_start
