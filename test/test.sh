#!/bin/sh

# Originally written by Matthew Lugg (mlugg).
# Script refactored by Dom Rodriguez (shymega).

set -eu

echo "*** BEGIN BUILD PROCESS ***"

echo "Cleaning previously compiled files..."
make clean

echo "Compiling gapxos..."
make

if [ ! -d "./iso" ]; then
  echo "Creating ISO build directory..."
  mkdir -p iso/boot/grub
fi

if [ ! -e "./iso/boot/kernel.bin" ]; then
  echo "Copying kernel to ISO build directory..."
  cp kernel/kernel iso/boot/kernel.bin
fi

if [ ! -e "./iso/boot/loader.bin" ]; then
  echo "Copying loader to ISO build directory..."
  cp loader/loader iso/boot/loader.bin
fi

if [ ! -e "./iso/boot/grub/grub.cfg" ]; then
  echo "Insert GRUB configuration to ISO build directory..."
  cp ./bootloader/grub.cfg ./iso/boot/grub/grub.cfg
fi

if [ -e "./os.iso" ]; then
  echo "Removing previously generated ISO..."
  rm ./os.iso
fi

if [ ! -e "./os.iso" ]; then
  echo "Create GRUB image..."
  grub-mkrescue -o ./os.iso ./iso/
fi

if [ -d "./iso/" ]; then
  echo "Remove ISO build directory.."
  rm -rf ./iso
fi

echo "Running gapxos in QEMU..."
qemu-system-x86_64 -boot d -cdrom os.iso -m 256 -s -d guest_errors &

sleep 2s

echo "GDB attached to QEMU emulator."
gdb --command="./dbg/gdb_start"

echo "*** FINISHED ***"

exit
