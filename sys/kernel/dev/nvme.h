#ifndef BRIGHTS_NVME_H
#define BRIGHTS_NVME_H

#include <stdint.h>
#include "block.h"
#include "../platform/x86_64/pci.h"

int brights_nvme_init(const brights_pci_device_t *dev);
brights_block_dev_t *brights_nvme_block(void);

#endif
