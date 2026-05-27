#ifndef BRIGHTS_PMEM_H
#define BRIGHTS_PMEM_H

#include <stdint.h>
#include "vm.h"
#include "../arch/x86_64/paging.h"

/* Initialize physical memory manager */
void brights_pmem_init(const brights_mem_region_t *regions, uint32_t count);

/* Allocate a single 4KB page (returns physical address) */
void *brights_pmem_alloc_page(void);

/* Free a previously allocated page */
void brights_pmem_free_page(void *phys_addr);

/* Allocate N contiguous pages (returns base physical address) */
void *brights_pmem_alloc_pages(uint32_t count);

/* Free N contiguous pages */
void brights_pmem_free_pages(void *phys_addr, uint32_t count);

/* Stats */
uint64_t brights_pmem_total_bytes(void);
uint64_t brights_pmem_free_bytes(void);
uint64_t brights_pmem_used_bytes(void);

/* Get number of free pages */
uint64_t brights_pmem_free_pages_count(void);

#endif
