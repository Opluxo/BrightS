#include "vm.h"
#include "pmem.h"
#include "../platform/x86_64/paging.h"

// UEFI fills these before calling brights_vm_init.
static uint64_t brights_mem_base;
static uint64_t brights_mem_size;

void brights_vm_bootstrap(uint64_t base, uint64_t size)
{
  brights_mem_base = base;
  brights_mem_size = size;
}

void brights_vm_init(void)
{
  if (brights_mem_size == 0) {
    // Fallback: 16MB after 1MB if memmap not available.
    brights_pmem_init(0x100000, 16 * 1024 * 1024);
  } else {
    brights_pmem_init(brights_mem_base, brights_mem_size);
  }
  brights_paging_init();
}
