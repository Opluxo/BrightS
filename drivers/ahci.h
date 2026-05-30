#ifndef BRIGHTS_AHCI_H
#define BRIGHTS_AHCI_H
#include <stdint.h>
#ifdef __i386__
#include "../arch/i386/pci.h"
#else
#include "../arch/x86_64/pci.h"
#endif
int brights_ahci_init(brights_pci_device_t *dev);
#endif
