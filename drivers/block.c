#include "block.h"

static brights_block_dev_t *root_dev;
static brights_block_dev_t *block_devs[BRIGHTS_MAX_BLOCK_DEVS];
static int block_dev_count = 0;

void brights_block_set_root(brights_block_dev_t *dev)
{
  root_dev = dev;
}

brights_block_dev_t *brights_block_root(void)
{
  return root_dev;
}

int brights_block_register(brights_block_dev_t *dev)
{
  if (!dev || block_dev_count >= BRIGHTS_MAX_BLOCK_DEVS) return -1;
  block_devs[block_dev_count] = dev;
  return block_dev_count++;
}

brights_block_dev_t *brights_block_get(int index)
{
  if (index < 0 || index >= block_dev_count) return 0;
  return block_devs[index];
}

int brights_block_count(void)
{
  return block_dev_count;
}
#include "block.h"

int brights_disk_read(uint64_t lba, void *buf, uint64_t nblocks)
{
  brights_block_dev_t *dev = brights_block_root();
  if (!dev || !dev->read) {
    return -1;
  }
  return dev->read(lba, buf, nblocks);
}

int brights_disk_write(uint64_t lba, const void *buf, uint64_t nblocks)
{
  brights_block_dev_t *dev = brights_block_root();
  if (!dev || !dev->write) {
    return -1;
  }
  return dev->write(lba, buf, nblocks);
}
