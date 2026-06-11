#include "uefi_memmap.h"

static void add_region(brights_uefi_memmap_info_t *out, uint64_t base, uint64_t size)
{
  if (!out || size == 0) {
    return;
  }

  out->total_bytes += size;

  for (uint32_t i = 0; i < out->region_count; ++i) {
    uint64_t cur_base = out->regions[i].base;
    uint64_t cur_end = cur_base + out->regions[i].size;
    uint64_t new_end = base + size;

    if (new_end < cur_base || base > cur_end) {
      continue;
    }

    if (base < cur_base) {
      cur_base = base;
    }
    if (new_end > cur_end) {
      cur_end = new_end;
    }
    out->regions[i].base = cur_base;
    out->regions[i].size = cur_end - cur_base;
    return;
  }

  if (out->region_count >= BRIGHTS_MAX_MEM_REGIONS) {
    return;
  }

  out->regions[out->region_count].base = base;
  out->regions[out->region_count].size = size;
  ++out->region_count;
}

int brights_uefi_parse_memmap(EFI_SYSTEM_TABLE *st, brights_uefi_memmap_info_t *out)
{
  uint64_t map_size = 0;
  uint64_t map_key = 0;
  uint64_t desc_size = 0;
  uint32_t desc_ver = 0;
  EFI_STATUS status;

  out->region_count = 0;
  out->total_bytes = 0;
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
    if (d->Type != 7) {
      continue;
    }
    add_region(out, d->PhysicalStart, d->NumberOfPages * 4096u);
  }

  out->map_key = map_key;
  out->desc_size = desc_size;
  out->desc_ver = desc_ver;
  return (out->region_count > 0) ? 0 : -1;
}
