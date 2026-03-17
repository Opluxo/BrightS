#ifndef BRIGHTS_AHCI_H
#define BRIGHTS_AHCI_H

#include <stdint.h>
#include "block.h"
#include "../platform/x86_64/pci.h"

int brights_ahci_init(const brights_pci_device_t *dev);
brights_block_dev_t *brights_ahci_block(void);

#endif
