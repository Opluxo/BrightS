#ifndef BRIGHTS_UEFI_MEMMAP_H
#define BRIGHTS_UEFI_MEMMAP_H

#include <stdint.h>
#include "uefi.h"
#include "../../core/vm.h"

typedef struct {
  brights_mem_region_t regions[BRIGHTS_MAX_MEM_REGIONS];
  uint32_t region_count;
  uint64_t total_bytes;
  uint64_t map_key;
  uint64_t desc_size;
  uint32_t desc_ver;
} brights_uefi_memmap_info_t;

int brights_uefi_parse_memmap(EFI_SYSTEM_TABLE *st, brights_uefi_memmap_info_t *out);

#endif
