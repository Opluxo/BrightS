#ifndef BRIGHTS_AHCI_H
#define BRIGHTS_AHCI_H

#include <stdint.h>
#include "block.h"
#include "../arch/x86_64/pci.h"

int brights_ahci_init(const brights_pci_device_t *dev);
brights_block_dev_t *brights_ahci_block(void);

#endif
#include "ahci.h"
#include "../kernel/printf.h"
#include "serial.h"
#include "../arch/x86_64/pci.h"

#define AHCI_HBA_CAP  0x00
#define AHCI_HBA_GHC  0x04
#define AHCI_HBA_PI   0x0C
#define AHCI_HBA_VS   0x10

#define AHCI_GHC_AE   (1u << 31)
#define AHCI_GHC_IE   (1u << 1)
#define AHCI_GHC_HR   (1u << 0)

#define AHCI_PORT_CLB  0x00
#define AHCI_PORT_FB   0x08
#define AHCI_PORT_IS   0x10
#define AHCI_PORT_IE   0x14
#define AHCI_PORT_CMD  0x18
#define AHCI_PORT_TFD  0x20
#define AHCI_PORT_SIG  0x24
#define AHCI_PORT_SSTS 0x28
#define AHCI_PORT_SCTL 0x2C
#define AHCI_PORT_SERR 0x30
#define AHCI_PORT_SACT 0x34
#define AHCI_PORT_CI   0x38

#define AHCI_PORT_DET_PRESENT 0x3
#define AHCI_PORT_IPM_ACTIVE  0x1

#define AHCI_PORT_CMD_ST  (1u << 0)
#define AHCI_PORT_CMD_FRE (1u << 4)
#define AHCI_PORT_CMD_FR  (1u << 14)
#define AHCI_PORT_CMD_CR  (1u << 15)

#define SATA_SIG_ATA 0x00000101

#define FIS_TYPE_REG_H2D 0x27

#define AHCI_CMD_READ_DMA_EXT  0x25
#define AHCI_CMD_WRITE_DMA_EXT 0x35

#define AHCI_SECTOR_SIZE 512
#define AHCI_MAX_PRDT 8

typedef struct {
  uint8_t fis_type;
  uint8_t pmport:4;
  uint8_t rsv0:3;
  uint8_t c:1;
  uint8_t command;
  uint8_t featurel;
  uint8_t lba0;
  uint8_t lba1;
  uint8_t lba2;
  uint8_t device;
  uint8_t lba3;
  uint8_t lba4;
  uint8_t lba5;
  uint8_t featureh;
  uint16_t count;
  uint8_t icc;
  uint8_t control;
  uint32_t rsv1;
} __attribute__((packed)) ahci_fis_h2d_t;

typedef struct {
  uint8_t cfl:5;
  uint8_t a:1;
  uint8_t w:1;
  uint8_t p:1;
  uint8_t r:1;
  uint8_t b:1;
  uint8_t c:1;
  uint8_t rsv0:1;
  uint8_t pmp:4;
  uint16_t prdtl;
  uint32_t prdbc;
  uint32_t ctba;
  uint32_t ctbau;
  uint32_t rsv1[4];
} __attribute__((packed)) ahci_cmd_header_t;

typedef struct {
  uint32_t dba;
  uint32_t dbau;
  uint32_t rsv0;
  uint32_t dbc:22;
  uint32_t rsv1:9;
  uint32_t i:1;
} __attribute__((packed)) ahci_prdt_t;

typedef struct {
  uint8_t cfis[64];
  uint8_t acmd[16];
  uint8_t rsv[48];
  ahci_prdt_t prdt[AHCI_MAX_PRDT];
} __attribute__((packed)) ahci_cmd_tbl_t;

static volatile uint8_t *ahci_mmio;
static brights_block_dev_t ahci_dev;
static int ahci_ready;
static uint8_t ahci_port;

static ahci_cmd_header_t cmd_hdr[32] __attribute__((aligned(1024)));
static ahci_cmd_tbl_t cmd_tbl[32] __attribute__((aligned(256)));
static uint8_t fis_buf[256] __attribute__((aligned(256)));

static void ahci_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

static void ahci_debug_hex32(uint32_t v)
{
  static const char hex[] = "0123456789ABCDEF";
  char buf[11];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 8; ++i) {
    buf[2 + i] = hex[(v >> ((7 - i) * 4)) & 0xF];
  }
  buf[10] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
}

static inline uint32_t mmio_read32(uint32_t off)
{
  return *(volatile uint32_t *)(ahci_mmio + off);
}

static inline void mmio_write32(uint32_t off, uint32_t v)
{
  *(volatile uint32_t *)(ahci_mmio + off) = v;
}

static inline uint32_t port_off(uint8_t port, uint32_t reg)
{
  return 0x100 + port * 0x80 + reg;
}

static void ahci_port_stop(uint8_t port)
{
  uint32_t cmd = mmio_read32(port_off(port, AHCI_PORT_CMD));
  cmd &= ~AHCI_PORT_CMD_ST;
  cmd &= ~AHCI_PORT_CMD_FRE;
  mmio_write32(port_off(port, AHCI_PORT_CMD), cmd);
  while (mmio_read32(port_off(port, AHCI_PORT_CMD)) & (AHCI_PORT_CMD_FR | AHCI_PORT_CMD_CR)) {
  }
}

static void ahci_port_start(uint8_t port)
{
  uint32_t cmd = mmio_read32(port_off(port, AHCI_PORT_CMD));
  cmd |= AHCI_PORT_CMD_FRE;
  cmd |= AHCI_PORT_CMD_ST;
  mmio_write32(port_off(port, AHCI_PORT_CMD), cmd);
}

static int ahci_exec(uint64_t lba, uint32_t count, void *buf, int write)
{
  ahci_cmd_header_t *hdr = &cmd_hdr[0];
  ahci_cmd_tbl_t *tbl = &cmd_tbl[0];

  hdr->cfl = sizeof(ahci_fis_h2d_t) / 4;
  hdr->w = write ? 1 : 0;

  uint32_t prdt_needed = (count * AHCI_SECTOR_SIZE + 4095) / 4096;
  if (prdt_needed == 0) {
    return -1;
  }
  if (prdt_needed > AHCI_MAX_PRDT) {
    return -1;
  }
  hdr->prdtl = (uint16_t)prdt_needed;
  hdr->ctba = (uint32_t)(uintptr_t)tbl;
  hdr->ctbau = (uint32_t)((uint64_t)(uintptr_t)tbl >> 32);

  for (uint32_t i = 0; i < prdt_needed; ++i) {
    uint64_t addr = (uint64_t)(uintptr_t)buf + (uint64_t)i * 4096;
    tbl->prdt[i].dba = (uint32_t)addr;
    tbl->prdt[i].dbau = (uint32_t)(addr >> 32);
    tbl->prdt[i].dbc = 4096 - 1;
    tbl->prdt[i].i = 1;
  }
  // Trim last PRDT to exact size.
  uint32_t total_bytes = count * AHCI_SECTOR_SIZE;
  uint32_t last_bytes = total_bytes - (prdt_needed - 1) * 4096;
  tbl->prdt[prdt_needed - 1].dbc = last_bytes - 1;

  ahci_fis_h2d_t *fis = (ahci_fis_h2d_t *)tbl->cfis;
  fis->fis_type = FIS_TYPE_REG_H2D;
  fis->c = 1;
  fis->command = write ? AHCI_CMD_WRITE_DMA_EXT : AHCI_CMD_READ_DMA_EXT;
  fis->lba0 = (uint8_t)(lba & 0xFF);
  fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
  fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
  fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
  fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
  fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);
  fis->device = 1 << 6;
  fis->count = (uint16_t)count;

  mmio_write32(port_off(ahci_port, AHCI_PORT_IS), 0xFFFFFFFF);
  mmio_write32(port_off(ahci_port, AHCI_PORT_CI), 1);
  while (mmio_read32(port_off(ahci_port, AHCI_PORT_CI)) & 1) {
    uint32_t tfd = mmio_read32(port_off(ahci_port, AHCI_PORT_TFD));
    if (tfd & 0x01) {
      return -1;
    }
  }
  return 0;
}

static int ahci_read(uint64_t lba, void *buf, uint64_t count)
{
  uint64_t sectors = (BRIGHTS_BLOCK_SIZE * count) / AHCI_SECTOR_SIZE;
  return ahci_exec(lba * (BRIGHTS_BLOCK_SIZE / AHCI_SECTOR_SIZE), (uint32_t)sectors, buf, 0);
}

static int ahci_write(uint64_t lba, const void *buf, uint64_t count)
{
  uint64_t sectors = (BRIGHTS_BLOCK_SIZE * count) / AHCI_SECTOR_SIZE;
  return ahci_exec(lba * (BRIGHTS_BLOCK_SIZE / AHCI_SECTOR_SIZE), (uint32_t)sectors, (void *)buf, 1);
}

int brights_ahci_init(const brights_pci_device_t *dev)
{
  brights_pci_enable_mmio_busmaster(dev);
  uint64_t bar = dev->bar[5];
  uint64_t mmio_base = bar & ~0xFULL;
  ahci_mmio = (volatile uint8_t *)(uintptr_t)mmio_base;
  ahci_debug("ahci: abar=");
  ahci_debug_hex32((uint32_t)mmio_base);
  ahci_debug("\r\n");

  mmio_write32(AHCI_HBA_GHC, AHCI_GHC_HR);
  while (mmio_read32(AHCI_HBA_GHC) & AHCI_GHC_HR) {
  }
  mmio_write32(AHCI_HBA_GHC, AHCI_GHC_AE);

  uint32_t pi = mmio_read32(AHCI_HBA_PI);
  ahci_debug("ahci: pi=");
  ahci_debug_hex32(pi);
  ahci_debug("\r\n");
  uint8_t found = 0;
  for (uint8_t p = 0; p < 32; ++p) {
    if ((pi & (1u << p)) == 0) {
      continue;
    }
    uint32_t sig = mmio_read32(port_off(p, AHCI_PORT_SIG));
    uint32_t ssts = mmio_read32(port_off(p, AHCI_PORT_SSTS));
    uint32_t det = ssts & 0xF;
    uint32_t ipm = (ssts >> 8) & 0xF;
    ahci_debug("ahci: port sig=");
    ahci_debug_hex32(sig);
    ahci_debug(" ssts=");
    ahci_debug_hex32(ssts);
    ahci_debug("\r\n");
    if (sig == SATA_SIG_ATA) {
      ahci_port = p;
      found = 1;
      break;
    }
    if (det == AHCI_PORT_DET_PRESENT && ipm == AHCI_PORT_IPM_ACTIVE) {
      ahci_debug("ahci: using active port despite nonstandard signature\r\n");
      ahci_port = p;
      found = 1;
      break;
    }
  }
  if (!found) {
    ahci_debug("ahci: no sata ata port found\r\n");
    return -1;
  }

  ahci_port_stop(ahci_port);
  mmio_write32(port_off(ahci_port, AHCI_PORT_CLB), (uint32_t)(uintptr_t)cmd_hdr);
  mmio_write32(port_off(ahci_port, AHCI_PORT_CLB) + 4, (uint32_t)((uint64_t)(uintptr_t)cmd_hdr >> 32));
  mmio_write32(port_off(ahci_port, AHCI_PORT_FB), (uint32_t)(uintptr_t)fis_buf);
  mmio_write32(port_off(ahci_port, AHCI_PORT_FB) + 4, (uint32_t)((uint64_t)(uintptr_t)fis_buf >> 32));
  ahci_port_start(ahci_port);

  ahci_dev.read = ahci_read;
  ahci_dev.write = ahci_write;
  ahci_dev.name = "ahci0";
  ahci_dev.type = BRIGHTS_BLOCK_DEV_AHCI;
  ahci_dev.total_blocks = 0;
  ahci_dev.block_size = BRIGHTS_BLOCK_SIZE;
  ahci_dev.ready = 1;
  ahci_ready = 1;
  brights_block_register(&ahci_dev);
  brights_block_set_root(&ahci_dev);
  return 0;
}

brights_block_dev_t *brights_ahci_block(void)
{
  return &ahci_dev;
}
