#ifndef BRIGHTS_UEFI_MEMMAP_H
#define BRIGHTS_UEFI_MEMMAP_H

#include <stdint.h>
#include "uefi.h"

typedef struct {
  uint64_t base;
  uint64_t size;
  uint64_t map_key;
  uint64_t desc_size;
  uint32_t desc_ver;
} brights_uefi_memmap_info_t;

int brights_uefi_parse_memmap(EFI_SYSTEM_TABLE *st, brights_uefi_memmap_info_t *out);

#endif
