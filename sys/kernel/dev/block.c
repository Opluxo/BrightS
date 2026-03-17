#include "block.h"

static brights_block_dev_t *root_dev;

void brights_block_set_root(brights_block_dev_t *dev)
{
  root_dev = dev;
}

brights_block_dev_t *brights_block_root(void)
{
  return root_dev;
}
