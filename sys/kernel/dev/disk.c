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
