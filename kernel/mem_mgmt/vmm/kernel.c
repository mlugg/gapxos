struct virtual_allocation {
  uintptr_t start;
  uintptr_t end;
  enum {
    ALLOCATED,
    FREE
  } type;
};

// Begins a virtual memory manager for the kernel given the (virtual) location of the PML4 table as created by the loader
void init_kern_vmm() {
  // We should first try to find where things are.
  // TODO: Find the stack! It should probably be in the PMM
  // Everything currently paged above 0xFFFF800000000000 is the kernel, so, uh, no touchy?

  uint64_t *pml4t;
  __asm__ volatile(
      "movq %%cr3, %0"
      : "=r" (pml4t)
  );

  uintptr_t max_virtual_kernel;
  
  for (int i = 256; i < 512; ++i) {
    uint64_t *pdpt = (void *)(pml4t[i] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

    uint64_t vaddr = i * 0x8000000000;
    if (!pdpt)
      continue;

    if (pml4t[i] & PAGE_SIZE) {
      max_virtual_kernel = vaddr;
      continue;
    }

    for (int j = 0; j < 512; ++j) {
      uint64_t *pdt = (void *)(pdpt[j] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

      vaddr += 0x40000000;

      if (!pdt)
        continue;

      if (pdpt[j] & PAGE_SIZE) {
        max_virtual_kernel = vaddr;
        continue;
      }

      for (int k = 0; k < 512; ++k) {
        uint64_t *pt = (void *)(pdt[k] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

        vaddr += 0x200000;

        if (!pt)
          continue;

        if (pdt[k] & PAGE_SIZE) {
          max_virtual_kernel = vaddr;
          continue;
        }

        for (int l = 0; l < 512; ++l) {
          uint64_t *page = (void *)(pt[l] & 0xFFFFFFFFFFFFF000); // Remove lower 12 bits

          vaddr += 0x1000;

          if (pt[l])
            max_virtual_kernel = vaddr;
        }
      }
    }
  }

  max_virtual_kernel += 0x1000;

  struct virtual_allocation kernel_allocation = {.start = max_virtual_kernel, .end = 0xffffffffffffffff, .type = FREE};

  // TODO: Add that to the AVL tree and we're off
  // TODO: Make space for the AVL tree and make sure the VMM knows it can't use it. Just set it as allocated so VMM memory can be dynamically allocated and freed
  // TODO: Recreate the page structures in less stupid places.
  // TODO: Actually don't, it doesn't really matter, they're all 4k long, and TLB misses aren't the end of the world
}
