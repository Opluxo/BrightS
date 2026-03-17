#ifndef BRIGHTS_RAMDISK_H
#define BRIGHTS_RAMDISK_H

#include <stdint.h>
#include "block.h"

void brights_ramdisk_init(uint8_t *base, uint64_t size);
brights_block_dev_t *brights_ramdisk_dev(void);

#endif
