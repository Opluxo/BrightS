#ifndef BRIGHTS_AHCI_H
#define BRIGHTS_AHCI_H
#include <stdint.h>
#include "../platform/x86_64/pci.h"
int brights_ahci_init(brights_pci_device_t *dev);
#endif
