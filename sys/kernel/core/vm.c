#include "vm.h"
#include "pmem.h"
#include "../arch/x86_64/paging.h"

static brights_mem_region_t brights_mem_regions[BRIGHTS_MAX_MEM_REGIONS];
static uint32_t brights_mem_region_count;

void brights_vm_bootstrap(const brights_mem_region_t *regions, uint32_t count)
{
  brights_mem_region_count = 0;
  if (!regions) {
    return;
  }

  for (uint32_t i = 0; i < count && brights_mem_region_count < BRIGHTS_MAX_MEM_REGIONS; ++i) {
    uint64_t base = regions[i].base;
    uint64_t end = regions[i].base + regions[i].size;

    if (end <= 0x100000u) {
      continue;
    }
    if (base < 0x100000u) {
      base = 0x100000u;
    }
    if (end <= base) {
      continue;
    }

    brights_mem_regions[brights_mem_region_count].base = base;
    brights_mem_regions[brights_mem_region_count].size = end - base;
    ++brights_mem_region_count;
  }
}

void brights_vm_init(void)
{
  if (brights_mem_region_count == 0) {
    static const brights_mem_region_t fallback_region = {0x100000u, 16u * 1024u * 1024u};
    brights_pmem_init(&fallback_region, 1);
  } else {
    brights_pmem_init(brights_mem_regions, brights_mem_region_count);
  }
  brights_paging_init();
}
