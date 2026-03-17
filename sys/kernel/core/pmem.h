#ifndef BRIGHTS_PMEM_H
#define BRIGHTS_PMEM_H

#include <stdint.h>

#define BRIGHTS_PAGE_SIZE 4096u

void brights_pmem_init(uint64_t base, uint64_t size);
void *brights_pmem_alloc_page(void);

#endif
