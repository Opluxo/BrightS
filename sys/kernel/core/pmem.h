#ifndef BRIGHTS_PMEM_H
#define BRIGHTS_PMEM_H

#include <stdint.h>
#include "vm.h"

#define BRIGHTS_PAGE_SIZE 4096u

void brights_pmem_init(const brights_mem_region_t *regions, uint32_t count);
void *brights_pmem_alloc_page(void);
uint64_t brights_pmem_total_bytes(void);
uint64_t brights_pmem_free_bytes(void);

#endif
