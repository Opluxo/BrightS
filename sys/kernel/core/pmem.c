#include "pmem.h"

typedef struct {
  uint64_t base;
  uint64_t end;
  uint64_t next;
} brights_pmem_region_state_t;

static brights_pmem_region_state_t pmem_regions[BRIGHTS_MAX_MEM_REGIONS];
static uint32_t pmem_region_count;
static uint64_t pmem_total_bytes;

static uint64_t align_up_page(uint64_t value)
{
  return (value + (BRIGHTS_PAGE_SIZE - 1u)) & ~(uint64_t)(BRIGHTS_PAGE_SIZE - 1u);
}

static uint64_t align_down_page(uint64_t value)
{
  return value & ~(uint64_t)(BRIGHTS_PAGE_SIZE - 1u);
}

void brights_pmem_init(const brights_mem_region_t *regions, uint32_t count)
{
  pmem_region_count = 0;
  pmem_total_bytes = 0;

  if (!regions) {
    return;
  }

  for (uint32_t i = 0; i < count && pmem_region_count < BRIGHTS_MAX_MEM_REGIONS; ++i) {
    if (regions[i].size < BRIGHTS_PAGE_SIZE) {
      continue;
    }

    uint64_t base = align_up_page(regions[i].base);
    uint64_t end = align_down_page(regions[i].base + regions[i].size);
    if (end <= base) {
      continue;
    }

    pmem_regions[pmem_region_count].base = base;
    pmem_regions[pmem_region_count].end = end;
    pmem_regions[pmem_region_count].next = base;
    pmem_total_bytes += end - base;
    ++pmem_region_count;
  }
}

void *brights_pmem_alloc_page(void)
{
  for (uint32_t i = 0; i < pmem_region_count; ++i) {
    if (pmem_regions[i].next + BRIGHTS_PAGE_SIZE > pmem_regions[i].end) {
      continue;
    }
    void *ret = (void *)(uintptr_t)pmem_regions[i].next;
    pmem_regions[i].next += BRIGHTS_PAGE_SIZE;
    return ret;
  }
  return 0;
}

uint64_t brights_pmem_total_bytes(void)
{
  return pmem_total_bytes;
}

uint64_t brights_pmem_free_bytes(void)
{
  uint64_t free_bytes = 0;
  for (uint32_t i = 0; i < pmem_region_count; ++i) {
    free_bytes += pmem_regions[i].end - pmem_regions[i].next;
  }
  return free_bytes;
}
