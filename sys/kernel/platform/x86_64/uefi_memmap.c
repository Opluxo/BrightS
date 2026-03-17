#include "uefi_memmap.h"

// Stage-1: pick the largest conventional memory region.
int brights_uefi_parse_memmap(EFI_SYSTEM_TABLE *st, brights_uefi_memmap_info_t *out)
{
  uint64_t map_size = 0;
  uint64_t map_key = 0;
  uint64_t desc_size = 0;
  uint32_t desc_ver = 0;
  EFI_STATUS status;

  out->base = 0;
  out->size = 0;
  out->map_key = 0;
  out->desc_size = 0;
  out->desc_ver = 0;

  status = st->BootServices->GetMemoryMap(&map_size, 0, &map_key, &desc_size, &desc_ver);
  if (status == 0 && map_size == 0) {
    return -1;
  }

  map_size += desc_size * 8;
  EFI_PHYSICAL_ADDRESS map_addr = 0;
  status = st->BootServices->AllocatePages(0, 0, (map_size + 4095) / 4096, &map_addr);
  if (status != 0) {
    return -1;
  }

  EFI_MEMORY_DESCRIPTOR *map = (EFI_MEMORY_DESCRIPTOR *)(uintptr_t)map_addr;
  status = st->BootServices->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_ver);
  if (status != 0) {
    return -1;
  }

  uint8_t *iter = (uint8_t *)map;
  uint64_t entries = map_size / desc_size;
  for (uint64_t i = 0; i < entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR *)(iter + i * desc_size);
    // Type 7 is EfiConventionalMemory.
    if (d->Type != 7) {
      continue;
    }
    uint64_t base = d->PhysicalStart;
    uint64_t size = d->NumberOfPages * 4096;
    if (size > out->size) {
      out->base = base;
      out->size = size;
    }
  }

  out->map_key = map_key;
  out->desc_size = desc_size;
  out->desc_ver = desc_ver;
  return 0;
}
