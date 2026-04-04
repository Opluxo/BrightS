#include "pmem.h"

/*
 * Bitmap-based physical memory allocator for up to 8GB+ RAM.
 *
 * Supports:
 * - Single page alloc/free
 * - Contiguous multi-page alloc/free
 * - O(n) first-fit allocation (scans bitmap)
 * - O(1) deallocation
 *
 * For i5-1135G7 + 8GB:
 *   8GB = 2,097,152 pages (4KB each)
 *   bitmap = 262,144 bytes = 256KB
 */

#define PMEM_MAX_PAGES   (2ULL * 1024 * 1024)  /* 2M pages = 8GB */
#define PMEM_BITMAP_SIZE (PMEM_MAX_PAGES / 8)   /* 256KB bitmap */
#define PMEM_BITMAP_ALIGN 8  /* Scan in 64-bit chunks */

/* Bitmap: 0 = free, 1 = used */
static uint64_t pmem_bitmap[PMEM_MAX_PAGES / 64];

/* Region tracking */
static uint64_t pmem_base;        /* Lowest usable physical address */
static uint64_t pmem_total_pages; /* Total pages tracked */
static uint64_t pmem_free_pages;  /* Currently free pages */

/* Kernel image reservation (will be set during init) */
static uint64_t pmem_kernel_start;
static uint64_t pmem_kernel_end;

static inline void bitmap_set(uint64_t page_idx)
{
  pmem_bitmap[page_idx / 64] |= (1ULL << (page_idx % 64));
}

static inline void bitmap_clear(uint64_t page_idx)
{
  pmem_bitmap[page_idx / 64] &= ~(1ULL << (page_idx % 64));
}

static inline int bitmap_test(uint64_t page_idx)
{
  return (pmem_bitmap[page_idx / 64] >> (page_idx % 64)) & 1;
}

static uint64_t align_up_page(uint64_t value)
{
  return (value + (BRIGHTS_PAGE_SIZE - 1ULL)) & ~(uint64_t)(BRIGHTS_PAGE_SIZE - 1ULL);
}

static uint64_t align_down_page(uint64_t value)
{
  return value & ~(uint64_t)(BRIGHTS_PAGE_SIZE - 1ULL);
}

void brights_pmem_init(const brights_mem_region_t *regions, uint32_t count)
{
  pmem_total_pages = 0;
  pmem_free_pages = 0;
  pmem_base = ~0ULL;

  if (!regions || count == 0) return;

  /* Find the lowest address and compute total pages */
  for (uint32_t i = 0; i < count; ++i) {
    uint64_t base = align_up_page(regions[i].base);
    uint64_t end = align_down_page(regions[i].base + regions[i].size);
    if (end <= base) continue;
    if (base < pmem_base) pmem_base = base;
    pmem_total_pages += (end - base) / BRIGHTS_PAGE_SIZE;
  }

  if (pmem_base == ~0ULL) return;

  /* Cap to max supported */
  if (pmem_total_pages > PMEM_MAX_PAGES) pmem_total_pages = PMEM_MAX_PAGES;

  /* Mark all pages as used initially */
  for (uint64_t i = 0; i < PMEM_MAX_PAGES / 64; ++i) {
    pmem_bitmap[i] = ~0ULL;
  }

  /* Mark usable regions as free */
  for (uint32_t i = 0; i < count; ++i) {
    uint64_t base = align_up_page(regions[i].base);
    uint64_t end = align_down_page(regions[i].base + regions[i].size);
    if (end <= base) continue;

    for (uint64_t addr = base; addr < end; addr += BRIGHTS_PAGE_SIZE) {
      uint64_t idx = (addr - pmem_base) / BRIGHTS_PAGE_SIZE;
      if (idx < PMEM_MAX_PAGES) {
        bitmap_clear(idx);
        ++pmem_free_pages;
      }
    }
  }

  /* Reserve first 1MB (legacy BIOS area, IVT, BDA, etc.) */
  for (uint64_t addr = 0; addr < 0x100000ULL && addr < pmem_base + pmem_total_pages * BRIGHTS_PAGE_SIZE; addr += BRIGHTS_PAGE_SIZE) {
    if (addr >= pmem_base) {
      uint64_t idx = (addr - pmem_base) / BRIGHTS_PAGE_SIZE;
      if (idx < PMEM_MAX_PAGES && !bitmap_test(idx)) {
        bitmap_set(idx);
        --pmem_free_pages;
      }
    }
  }
}

/* Mark a kernel region as reserved (called during early boot) */
void brights_pmem_reserve(uint64_t start, uint64_t end)
{
  start = align_down_page(start);
  end = align_up_page(end);
  for (uint64_t addr = start; addr < end; addr += BRIGHTS_PAGE_SIZE) {
    if (addr >= pmem_base) {
      uint64_t idx = (addr - pmem_base) / BRIGHTS_PAGE_SIZE;
      if (idx < PMEM_MAX_PAGES && !bitmap_test(idx)) {
        bitmap_set(idx);
        --pmem_free_pages;
      }
    }
  }
}

void *brights_pmem_alloc_page(void)
{
  /* Fast scan: use bit-scan-forward (BSF) on each 64-bit word */
  for (uint64_t i = 0; i < PMEM_MAX_PAGES / 64; ++i) {
    uint64_t word = pmem_bitmap[i];
    if (word != ~0ULL) {
      /* At least one bit is 0 (free). Find it with BSF on inverted word. */
      uint64_t free_bits = ~word;
      if (free_bits == 0) continue; /* Shouldn't happen, but safety */
      uint64_t bit;
      __asm__ __volatile__("bsf %1, %0" : "=r"(bit) : "r"(free_bits) : "cc");
      uint64_t idx = i * 64 + bit;
      if (idx >= pmem_total_pages) return 0;
      bitmap_set(idx);
      --pmem_free_pages;
      return (void *)(uintptr_t)(pmem_base + idx * BRIGHTS_PAGE_SIZE);
    }
  }
  return 0;
}

void brights_pmem_free_page(void *phys_addr)
{
  if (!phys_addr) return;
  uint64_t addr = (uint64_t)(uintptr_t)phys_addr;
  if (addr < pmem_base) return;
  uint64_t idx = (addr - pmem_base) / BRIGHTS_PAGE_SIZE;
  if (idx >= PMEM_MAX_PAGES) return;
  if (bitmap_test(idx)) {
    bitmap_clear(idx);
    ++pmem_free_pages;
  }
}

void *brights_pmem_alloc_pages(uint32_t count)
{
  if (count == 0) return 0;
  if (count == 1) return brights_pmem_alloc_page();

  /* First-fit contiguous search */
  uint32_t found = 0;
  uint64_t start_idx = 0;

  for (uint64_t i = 0; i < PMEM_MAX_PAGES; ++i) {
    if (!bitmap_test(i)) {
      if (found == 0) start_idx = i;
      ++found;
      if (found == count) {
        /* Mark all pages as used */
        for (uint64_t j = start_idx; j < start_idx + count; ++j) {
          bitmap_set(j);
        }
        pmem_free_pages -= count;
        return (void *)(uintptr_t)(pmem_base + start_idx * BRIGHTS_PAGE_SIZE);
      }
    } else {
      found = 0;
    }
  }
  return 0; /* No contiguous block found */
}

void brights_pmem_free_pages(void *phys_addr, uint32_t count)
{
  if (!phys_addr || count == 0) return;
  uint64_t addr = (uint64_t)(uintptr_t)phys_addr;
  if (addr < pmem_base) return;
  uint64_t start_idx = (addr - pmem_base) / BRIGHTS_PAGE_SIZE;

  for (uint32_t i = 0; i < count; ++i) {
    uint64_t idx = start_idx + i;
    if (idx < PMEM_MAX_PAGES && bitmap_test(idx)) {
      bitmap_clear(idx);
      ++pmem_free_pages;
    }
  }
}

uint64_t brights_pmem_total_bytes(void)
{
  return pmem_total_pages * BRIGHTS_PAGE_SIZE;
}

uint64_t brights_pmem_free_bytes(void)
{
  return pmem_free_pages * BRIGHTS_PAGE_SIZE;
}

uint64_t brights_pmem_used_bytes(void)
{
  return (pmem_total_pages - pmem_free_pages) * BRIGHTS_PAGE_SIZE;
}

uint64_t brights_pmem_free_pages_count(void)
{
  return pmem_free_pages;
}
