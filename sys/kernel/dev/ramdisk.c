#include "ramdisk.h"

static uint8_t *ram_base;
static uint64_t ram_size;
static brights_block_dev_t ram_dev;

static int ramdisk_read(uint64_t lba, void *buf, uint64_t count)
{
  uint64_t offset = lba * BRIGHTS_BLOCK_SIZE;
  uint64_t bytes = count * BRIGHTS_BLOCK_SIZE;
  if (offset + bytes > ram_size) {
    return -1;
  }
  uint8_t *dst = (uint8_t *)buf;
  const uint8_t *src = ram_base + offset;
  for (uint64_t i = 0; i < bytes; ++i) {
    dst[i] = src[i];
  }
  return 0;
}

static int ramdisk_write(uint64_t lba, const void *buf, uint64_t count)
{
  uint64_t offset = lba * BRIGHTS_BLOCK_SIZE;
  uint64_t bytes = count * BRIGHTS_BLOCK_SIZE;
  if (offset + bytes > ram_size) {
    return -1;
  }
  uint8_t *dst = ram_base + offset;
  const uint8_t *src = (const uint8_t *)buf;
  for (uint64_t i = 0; i < bytes; ++i) {
    dst[i] = src[i];
  }
  return 0;
}

void brights_ramdisk_init(uint8_t *base, uint64_t size)
{
  ram_base = base;
  ram_size = size;
  ram_dev.read = ramdisk_read;
  ram_dev.write = ramdisk_write;
  ram_dev.name = "ram0";
  ram_dev.type = BRIGHTS_BLOCK_DEV_RAMDISK;
  ram_dev.total_blocks = size / BRIGHTS_BLOCK_SIZE;
  ram_dev.block_size = BRIGHTS_BLOCK_SIZE;
  ram_dev.ready = 1;
  brights_block_register(&ram_dev);
}

brights_block_dev_t *brights_ramdisk_dev(void)
{
  return &ram_dev;
}
