#ifndef BRIGHTS_NVME_H
#define BRIGHTS_NVME_H
#include <stdint.h>
#include "../arch/x86_64/pci.h"
int brights_nvme_init(brights_pci_device_t *dev);
#endif
