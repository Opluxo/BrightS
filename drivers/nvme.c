#ifndef BRIGHTS_NVME_H
#define BRIGHTS_NVME_H

#include <stdint.h>
#include "block.h"
#include "../arch/x86_64/pci.h"

int brights_nvme_init(const brights_pci_device_t *dev);
brights_block_dev_t *brights_nvme_block(void);

#endif
#include "nvme.h"
#include "../kernel/printf.h"
#include "serial.h"
#include "../arch/x86_64/pci.h"

#define NVME_REG_CAP   0x0000
#define NVME_REG_VS    0x0008
#define NVME_REG_CC    0x0014
#define NVME_REG_CSTS  0x001C
#define NVME_REG_AQA   0x0024
#define NVME_REG_ASQ   0x0028
#define NVME_REG_ACQ   0x0030
#define NVME_REG_DB    0x1000

#define NVME_CC_EN     (1u << 0)
#define NVME_CSTS_RDY  (1u << 0)

#define NVME_ADMIN_Q_DEPTH 16
#define NVME_IO_Q_DEPTH 64

#define NVME_CID_MAX 0xFFFC

typedef struct {
  uint8_t opc;
  uint8_t flags;
  uint16_t cid;
  uint32_t nsid;
  uint64_t rsvd2;
  uint64_t mptr;
  uint64_t prp1;
  uint64_t prp2;
  uint32_t cdw10;
  uint32_t cdw11;
  uint32_t cdw12;
  uint32_t cdw13;
  uint32_t cdw14;
  uint32_t cdw15;
} __attribute__((packed)) nvme_cmd_t;

typedef struct {
  uint32_t dw0;
  uint32_t dw1;
  uint16_t sq_head;
  uint16_t sq_id;
  uint16_t cid;
  uint16_t status;
} __attribute__((packed)) nvme_cqe_t;

typedef struct {
  uint32_t nsze;
  uint32_t ncap;
  uint32_t nuse;
  uint8_t rsvd[16];
  uint8_t flbas;
  uint8_t rsvd2[2];
  uint8_t nlbaf;
  uint8_t rsvd3[3];
  struct { uint16_t ms; uint8_t ds; uint8_t rp; } lbaf[16];
} __attribute__((packed)) nvme_identify_ns_t;

static volatile uint32_t *nvme_mmio;
static brights_block_dev_t nvme_dev;
static int nvme_ready;
static uint32_t nvme_db_stride;
static uint32_t nvme_block_shift = 9; // default 512

static nvme_cmd_t admin_sq[NVME_ADMIN_Q_DEPTH] __attribute__((aligned(4096)));
static nvme_cqe_t admin_cq[NVME_ADMIN_Q_DEPTH] __attribute__((aligned(4096)));
static nvme_cmd_t io_sq[NVME_IO_Q_DEPTH] __attribute__((aligned(4096)));
static nvme_cqe_t io_cq[NVME_IO_Q_DEPTH] __attribute__((aligned(4096)));
static uint64_t io_prp2[NVME_IO_Q_DEPTH][512] __attribute__((aligned(4096)));

static uint16_t admin_sq_tail;
static uint16_t admin_cq_head;
static uint8_t admin_phase = 1;

static uint16_t io_sq_tail;
static uint16_t io_cq_head;
static uint8_t io_phase = 1;
static uint16_t io_cid_next = 1;

static inline void mmio_write32(uint32_t off, uint32_t v)
{
  nvme_mmio[off / 4] = v;
}

static inline uint32_t mmio_read32(uint32_t off)
{
  return nvme_mmio[off / 4];
}

static inline uint64_t mmio_read64(uint32_t off)
{
  uint64_t lo = nvme_mmio[off / 4];
  uint64_t hi = nvme_mmio[off / 4 + 1];
  return lo | (hi << 32);
}

static inline uint32_t nvme_db_offset(uint16_t qid, uint8_t is_cq)
{
  return NVME_REG_DB + ((2 * qid + is_cq) * (4u << nvme_db_stride));
}

static uint16_t alloc_cid(void)
{
  uint16_t cid = io_cid_next++;
  if (io_cid_next >= NVME_CID_MAX) {
    io_cid_next = 1;
  }
  return cid;
}

static void nvme_submit_admin(const nvme_cmd_t *cmd)
{
  admin_sq[admin_sq_tail] = *cmd;
  admin_sq_tail = (admin_sq_tail + 1) % NVME_ADMIN_Q_DEPTH;
  mmio_write32(nvme_db_offset(0, 0), admin_sq_tail);
}

static int nvme_poll_admin(uint16_t cid)
{
  for (;;) {
    nvme_cqe_t *cqe = &admin_cq[admin_cq_head];
    uint8_t phase = cqe->status & 1;
    if (phase != admin_phase) {
      continue;
    }
    if (cqe->cid != cid) {
      return -1;
    }
    admin_cq_head = (admin_cq_head + 1) % NVME_ADMIN_Q_DEPTH;
    if (admin_cq_head == 0) {
      admin_phase ^= 1;
    }
    mmio_write32(nvme_db_offset(0, 1), admin_cq_head);
    return (cqe->status >> 1) & 0x7FFF;
  }
}

static int nvme_identify(uint32_t nsid, void *buf)
{
  nvme_cmd_t cmd = {0};
  cmd.opc = 0x06;
  cmd.cid = 1;
  cmd.nsid = nsid;
  cmd.prp1 = (uint64_t)(uintptr_t)buf;
  cmd.cdw10 = (nsid == 0) ? 1 : 0; // CNS: 1=controller,0=namespace
  nvme_submit_admin(&cmd);
  return nvme_poll_admin(cmd.cid);
}

static int nvme_create_io_queues(void)
{
  // Create I/O CQ (opcode 0x05)
  nvme_cmd_t cq = {0};
  cq.opc = 0x05;
  cq.cid = 2;
  cq.prp1 = (uint64_t)(uintptr_t)io_cq;
  cq.cdw10 = (NVME_IO_Q_DEPTH - 1) | (1u << 16); // qid=1
  cq.cdw11 = 1; // contiguous, interrupts disabled
  nvme_submit_admin(&cq);
  if (nvme_poll_admin(cq.cid) != 0) {
    return -1;
  }

  // Create I/O SQ (opcode 0x01)
  nvme_cmd_t sq = {0};
  sq.opc = 0x01;
  sq.cid = 3;
  sq.prp1 = (uint64_t)(uintptr_t)io_sq;
  sq.cdw10 = (NVME_IO_Q_DEPTH - 1) | (1u << 16); // qid=1
  sq.cdw11 = (1u << 16) | 1; // cqid=1, contiguous
  nvme_submit_admin(&sq);
  if (nvme_poll_admin(sq.cid) != 0) {
    return -1;
  }
  return 0;
}

static int nvme_submit_io(uint8_t opc, uint64_t lba, uint16_t nblocks, void *buf)
{
  nvme_cmd_t cmd = {0};
  cmd.opc = opc; // 0x02 read, 0x01 write
  cmd.cid = alloc_cid();
  cmd.nsid = 1;
  cmd.prp1 = (uint64_t)(uintptr_t)buf;
  cmd.cdw10 = (uint32_t)lba;
  cmd.cdw11 = (uint32_t)(lba >> 32);
  cmd.cdw12 = (uint32_t)(nblocks - 1);

  if (nblocks > 1) {
    uint64_t *prp = io_prp2[cmd.cid % NVME_IO_Q_DEPTH];
    uint64_t addr = (uint64_t)(uintptr_t)buf + 4096;
    uint32_t prp_entries = nblocks - 1;
    if (prp_entries > 512) {
      return -1;
    }
    for (uint32_t i = 0; i < prp_entries; ++i) {
      prp[i] = addr + (uint64_t)i * 4096;
    }
    cmd.prp2 = (uint64_t)(uintptr_t)prp;
  }

  io_sq[io_sq_tail] = cmd;
  io_sq_tail = (io_sq_tail + 1) % NVME_IO_Q_DEPTH;
  mmio_write32(nvme_db_offset(1, 0), io_sq_tail);

  for (;;) {
    nvme_cqe_t *cqe = &io_cq[io_cq_head];
    uint8_t phase = cqe->status & 1;
    if (phase != io_phase) {
      continue;
    }
    if (cqe->cid != cmd.cid) {
      return -1;
    }
    io_cq_head = (io_cq_head + 1) % NVME_IO_Q_DEPTH;
    if (io_cq_head == 0) {
      io_phase ^= 1;
    }
    mmio_write32(nvme_db_offset(1, 1), io_cq_head);
    return (cqe->status >> 1) & 0x7FFF;
  }
}

static int nvme_read(uint64_t lba, void *buf, uint64_t count)
{
  uint64_t blocks_per_io = BRIGHTS_BLOCK_SIZE >> nvme_block_shift;
  if (blocks_per_io == 0) {
    return -1;
  }
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t dev_lba = lba * blocks_per_io + i * blocks_per_io;
    if (nvme_submit_io(0x02, dev_lba, (uint16_t)blocks_per_io, (uint8_t *)buf + i * BRIGHTS_BLOCK_SIZE) != 0) {
      return -1;
    }
  }
  return 0;
}

static int nvme_write(uint64_t lba, const void *buf, uint64_t count)
{
  uint64_t blocks_per_io = BRIGHTS_BLOCK_SIZE >> nvme_block_shift;
  if (blocks_per_io == 0) {
    return -1;
  }
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t dev_lba = lba * blocks_per_io + i * blocks_per_io;
    if (nvme_submit_io(0x01, dev_lba, (uint16_t)blocks_per_io, (void *)((const uint8_t *)buf + i * BRIGHTS_BLOCK_SIZE)) != 0) {
      return -1;
    }
  }
  return 0;
}

int brights_nvme_init(const brights_pci_device_t *dev)
{
  brights_pci_enable_mmio_busmaster(dev);
  uint64_t bar = dev->bar[0];
  if ((bar & 0x6) == 0x4) { // 64-bit BAR
    bar |= ((uint64_t)dev->bar[1]) << 32;
  }
  uint64_t mmio_base = bar & ~0xFULL;
  nvme_mmio = (volatile uint32_t *)(uintptr_t)mmio_base; // identity map assumed

  uint64_t cap = mmio_read64(NVME_REG_CAP);
  nvme_db_stride = (cap >> 32) & 0xF;

  mmio_write32(NVME_REG_CC, 0);
  while (mmio_read32(NVME_REG_CSTS) & NVME_CSTS_RDY) {
  }

  mmio_write32(NVME_REG_AQA, ((NVME_ADMIN_Q_DEPTH - 1) << 16) | (NVME_ADMIN_Q_DEPTH - 1));
  mmio_write32(NVME_REG_ASQ, (uint32_t)(uintptr_t)admin_sq);
  mmio_write32(NVME_REG_ASQ + 4, (uint32_t)((uint64_t)(uintptr_t)admin_sq >> 32));
  mmio_write32(NVME_REG_ACQ, (uint32_t)(uintptr_t)admin_cq);
  mmio_write32(NVME_REG_ACQ + 4, (uint32_t)((uint64_t)(uintptr_t)admin_cq >> 32));

  mmio_write32(NVME_REG_CC, NVME_CC_EN | (6u << 20)); // I/O CQES/SQES = 64 bytes
  while ((mmio_read32(NVME_REG_CSTS) & NVME_CSTS_RDY) == 0) {
  }

  if (nvme_identify(0, (void *)admin_sq) != 0) {
    return -1;
  }
  nvme_identify_ns_t *ns = (nvme_identify_ns_t *)admin_sq;
  uint8_t lbaf = ns->flbas & 0xF;
  nvme_block_shift = ns->lbaf[lbaf].ds;

  if (nvme_identify(1, (void *)admin_sq) != 0) {
    return -1;
  }

  if (nvme_create_io_queues() != 0) {
    return -1;
  }

  nvme_dev.read = nvme_read;
  nvme_dev.write = nvme_write;
  nvme_dev.name = "nvme0";
  nvme_dev.type = BRIGHTS_BLOCK_DEV_NVME;
  nvme_dev.total_blocks = 0; /* Could be queried from namespace identify */
  nvme_dev.block_size = BRIGHTS_BLOCK_SIZE;
  nvme_dev.ready = 1;
  nvme_ready = 1;
  brights_block_register(&nvme_dev);
  brights_block_set_root(&nvme_dev);
  return 0;
}

brights_block_dev_t *brights_nvme_block(void)
{
  return &nvme_dev;
}
