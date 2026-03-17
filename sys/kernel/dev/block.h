#ifndef BRIGHTS_BLOCK_H
#define BRIGHTS_BLOCK_H

#include <stdint.h>

#define BRIGHTS_BLOCK_SIZE 4096u

typedef struct {
  int (*read)(uint64_t lba, void *buf, uint64_t count);
  int (*write)(uint64_t lba, const void *buf, uint64_t count);
} brights_block_dev_t;

void brights_block_set_root(brights_block_dev_t *dev);
brights_block_dev_t *brights_block_root(void);

#endif
