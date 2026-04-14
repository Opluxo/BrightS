#ifndef BRIGHTS_VM_H
#define BRIGHTS_VM_H

#include <stdint.h>

#define BRIGHTS_MAX_MEM_REGIONS 32u

typedef struct {
  uint64_t base;
  uint64_t size;
} brights_mem_region_t;

void brights_vm_init(void);
void brights_vm_bootstrap(const brights_mem_region_t *regions, uint32_t count);

#endif
