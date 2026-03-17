#include "pmem.h"

static uint64_t pmem_base;
static uint64_t pmem_end;
static uint64_t pmem_next;

void brights_pmem_init(uint64_t base, uint64_t size)
{
  pmem_base = base;
  pmem_end = base + size;
  pmem_next = base;
}

void *brights_pmem_alloc_page(void)
{
  if (pmem_next + BRIGHTS_PAGE_SIZE > pmem_end) {
    return 0;
  }
  void *ret = (void *)(uintptr_t)pmem_next;
  pmem_next += BRIGHTS_PAGE_SIZE;
  return ret;
}
