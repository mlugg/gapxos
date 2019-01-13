.POSIX:
.PHONY: all clean iso test

GRUB := ""

# Does `grub2-mkrescue` exist?
# If not, use `grub-mkrescue`.
#
# If it does, use `grub2-mkconfig`.
ifeq (,$(shell command -v grub2-mkrescue 2> /dev/null))
	GRUB := "grub-mkrescue"
else
	GRUB := "grub2-mkrescue"
endif

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
	${GRUB} -o ./os.iso ./iso
	rm -rf ./iso

test: iso
	qemu-system-x86_64 -boot d -cdrom os.iso -m 64 -s -d guest_errors &
	sleep 2s
	gdb -x ./dbg/gdb_start
