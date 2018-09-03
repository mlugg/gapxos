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

- Kernel virtual memory management using a self-balancing binary tree
- System calls
- Filesystem structure
- Task scheduling
- User-space virtual memory management
