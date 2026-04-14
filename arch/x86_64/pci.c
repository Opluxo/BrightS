#include "pci.h"
#include "io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_cfg_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
  return (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC);
}

static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
  outl(PCI_CONFIG_ADDRESS, pci_cfg_addr(bus, dev, func, reg));
  return inl(PCI_CONFIG_DATA);
}

static void pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t value)
{
  outl(PCI_CONFIG_ADDRESS, pci_cfg_addr(bus, dev, func, reg));
  outl(PCI_CONFIG_DATA, value);
}

uint32_t brights_pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
  return pci_read32(bus, dev, func, reg);
}

void brights_pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t value)
{
  pci_write32(bus, dev, func, reg, value);
}

void brights_pci_enable_mmio_busmaster(const brights_pci_device_t *dev)
{
  uint32_t cmd = pci_read32(dev->bus, dev->dev, dev->func, 0x04);
  cmd |= (1u << 1) | (1u << 2);
  pci_write32(dev->bus, dev->dev, dev->func, 0x04, cmd);
}

int brights_pci_scan(brights_pci_device_t *out, uint32_t max)
{
  uint32_t count = 0;
  for (uint16_t bus = 0; bus < 256; ++bus) {
    for (uint8_t dev = 0; dev < 32; ++dev) {
      for (uint8_t func = 0; func < 8; ++func) {
        uint32_t id = pci_read32(bus, dev, func, 0x00);
        if ((id & 0xFFFF) == 0xFFFF) {
          if (func == 0) {
            break;
          }
          continue;
        }
        if (count >= max) {
          return (int)count;
        }
        brights_pci_device_t *d = &out[count++];
        d->bus = (uint8_t)bus;
        d->dev = dev;
        d->func = func;
        d->vendor_id = (uint16_t)(id & 0xFFFF);
        d->device_id = (uint16_t)(id >> 16);
        uint32_t classreg = pci_read32(bus, dev, func, 0x08);
        d->class_code = (uint8_t)(classreg >> 24);
        d->subclass = (uint8_t)(classreg >> 16);
        d->prog_if = (uint8_t)(classreg >> 8);
        uint32_t hdr = pci_read32(bus, dev, func, 0x0C);
        d->header_type = (uint8_t)((hdr >> 16) & 0x7F);
        for (int i = 0; i < 6; ++i) {
          d->bar[i] = pci_read32(bus, dev, func, 0x10 + i * 4);
        }
      }
    }
  }
  return (int)count;
}
