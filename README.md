# gapxos

gapxos (Generic All-Purpose eXtensible Operating System) is an operating
system built for the 64-bit amd64 architecture. It is a microkernel
design, where most or all drivers are run as userspace applications.

gapxos is compliant with GNU Multiboot 2, and the `iso` target in this
repository will generate a bootable ISO image using GRUB 2. If you wish
to use another loader, it is important that the `kernel` file is loaded
as a module.

gapxos is NOT meant to be a POSIX-compliant operating system. It is also
not compatible with any legacy technologies such as APM and 32-bit
processors.

The system is currently in a very early state, and as such cannot be
used for most applications. Here are some important features on the
roadmap:

- Interrupt handling
- System calls
- Filesystem structure
- Task scheduling

## System Outline

gapxos boots using a Multiboot 2 compliant bootloader, such as GNU GRUB.
As Multiboot only supports running 32-bit images in x86 protected mode,
it boots a small x86 "loader" and loads the actual 64-bit kernel in a
boot module from an ELF file. The loader sets up a 4k stack and
simple GDT. It then traverses the Multiboot structure to find the
loaded kernel, and parses the ELF file, setting up the appropriate page
tables. It also identity maps the first 8MiB of memory, so the loader
can keep executing when it later switches to long mode - this is NOT
CERTAIN to work, as Multiboot makes no guarantees about the position in
memory of the loaded image, and should be changed in future. The kernel
is mapped in the higher half of virtual memory, in address
0xffff800000000000 and above. After it finishes parsing the ELF file,
the loader prepares to jump to 64-bit code. It enables PAE, and switches
to IA32e compatibility mode, with paging enabled. It loads a small x64
GDT, and jumps to x64 code. It sets several registers to act as
parameters for the `kernel_main` function, and jumps to the loaded
kernel code.

The `kernel_main` function take several arguments. The first is the
location in memory of the Multiboot structure, and the rest describe the
location of reserved areas of memory which should not be marked as
available: specifically, the stack, and the kernel code itself. Firstly,
this function recreates the GDT; it is essentially identical to the one
created by the loader, but must be recreated as the existing one could
be overwritten later.  first job of the kernel is to parse the Multiboot
structure and get any useful information from it. Every element of the
structure is traversed, and 4 types of tags are actually read: the
memory map, any present framebuffers, and the RSDP for either version of
ACPI. In future, this will likely also read a second boot module to use
as the initramfs.  After reading the entire multiboot structure and e.g.
validating ACPI checksums, the system begins to initialize the physical
memory manager.  The PMM is implemented very simply as a stack of free
4k memory pages, so the system first reads the memory map to identify
how much memory will be needed to store the stack itself. Once this is
calculated, it traverses the map again to find a range of memory which
is of sufficient size to store this list and is not already taken up by
the program stack, the kernel code, the page tables or the memory map
itself, which is the only part of the Multiboot structure still required
by the system at this point. After the stack of free memory pages is
created and populated, the system finishes its inital boot preparations
and runs the `system_main` function.

The `system_main` function takes a single argument, which is a structure
in memory containing information such as the ACPI RSDT. This function
starts by setting up a virtual memory manager for the kernel memory.
The virtual memory manager is implemented using a self-balancing binary
tree (AVL tree) to store free and allocated ranges of memory. This
virtual memory manager begins at a virtual address which is a safe range
above the kernel code, such as 0xffff810000000000, and runs until a long
range later, but not to the end of memory so as to keep the stack
uninterrupted (the precise numbers do not really have an effect as long
as there is a sufficient range of virtual memory). It next begins to
read ACPI tables.
