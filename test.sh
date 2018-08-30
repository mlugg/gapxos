#!/bin/sh
make clean
make
rm -rf iso
mkdir iso
mkdir iso/boot
cp kernel/kernel iso/boot/kernel.bin
cp loader/loader iso/boot/loader.bin
mkdir iso/boot/grub
cat grub.cfg > iso/boot/grub/grub.cfg
rm os.iso
grub-mkrescue -o os.iso iso/
rm -rf iso/

qemu-system-x86_64 -cdrom os.iso -m 256 -s -d guest_errors &

sleep 1

gdb --command="gdb_start"
