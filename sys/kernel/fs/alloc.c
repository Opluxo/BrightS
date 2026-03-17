#include <stdint.h>

static uint64_t next_inode_no = 2;
static uint64_t next_block_no = 1;

uint64_t brights_v6_alloc_inode(void)
{
  return next_inode_no++;
}

uint64_t brights_v6_alloc_block(void)
{
  return next_block_no++;
}
