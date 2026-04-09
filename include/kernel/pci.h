#ifndef BRIGHTS_X86_64_PCI_H
#define BRIGHTS_X86_64_PCI_H

#include <stdint.h>

typedef struct {
  uint8_t bus;
  uint8_t dev;
  uint8_t func;
  uint16_t vendor_id;
  uint16_t device_id;
  uint8_t class_code;
  uint8_t subclass;
  uint8_t prog_if;
  uint8_t header_type;
  uint32_t bar[6];
} brights_pci_device_t;

int brights_pci_scan(brights_pci_device_t *out, uint32_t max);
uint32_t brights_pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void brights_pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t value);
void brights_pci_enable_mmio_busmaster(const brights_pci_device_t *dev);

#endif
