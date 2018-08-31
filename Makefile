.POSIX:
.PHONY: all clean test

all:
	+$(MAKE) -C loader
	+$(MAKE) -C kernel

clean:
	+$(MAKE) clean -C loader
	+$(MAKE) clean -C kernel
	rm -rf ./iso
	rm -rf ./os.iso

test:
	./test/test.sh

iso:
	+${MAKE} all
	mkdir -p ./iso/boot/grub
	cp ./kernel/kernel ./iso/boot/kernel.bin
	cp ./loader/loader ./iso/boot/loader.bin
	cp ./bootloader/grub.cfg ./iso/boot/grub/grub.cfg
	grub-mkrescue -o ./os.iso ./iso
	rm -rf ./iso
