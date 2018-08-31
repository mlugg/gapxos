.POSIX:
.PHONY: all clean

all:
	+$(MAKE) -C loader
	+$(MAKE) -C kernel

clean:
	+$(MAKE) clean -C loader
	+$(MAKE) clean -C kernel
	rm -rf ./iso
	rm -rf ./os.iso

