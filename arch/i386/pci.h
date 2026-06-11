#ifndef BRIGHTS_I386_PCI_H
#define BRIGHTS_I386_PCI_H

#include <stdint.h>
#include "io.h"

#define BRIGHTS_PCI_CONFIG_ADDR 0xCF8
#define BRIGHTS_PCI_CONFIG_DATA 0xCFC

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

static inline uint32_t brights_pci_read(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
    uint32_t addr = (1u << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(BRIGHTS_PCI_CONFIG_ADDR, addr);
    return inl(BRIGHTS_PCI_CONFIG_DATA);
}

static inline uint16_t brights_pci_read_vendor(uint32_t bus, uint32_t slot, uint32_t func)
{
    return (uint16_t)brights_pci_read(bus, slot, func, 0);
}

static inline uint16_t brights_pci_read_device(uint32_t bus, uint32_t slot, uint32_t func)
{
    return (uint16_t)(brights_pci_read(bus, slot, func, 0) >> 16);
}

int brights_pci_scan(brights_pci_device_t *out, uint32_t max);
uint32_t brights_pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void brights_pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t value);
void brights_pci_enable_mmio_busmaster(const brights_pci_device_t *dev);

#endif
