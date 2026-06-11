#include "ioapic.h"
#include <stdint.h>

/* IOAPIC registers */
#define IOAPIC_REG_ID     0x00
#define IOAPIC_REG_VER    0x01
#define IOAPIC_REG_ARB    0x02
#define IOAPIC_REG_REDTBL 0x10  /* Redirection table base */

/* Redirection entry bits */
#define IOAPIC_REDTBL_MASKED  (1ULL << 16)
#define IOAPIC_REDTBL_LEVEL   (1ULL << 15)
#define IOAPIC_REDTBL_REMOTE  (1ULL << 14)
#define IOAPIC_REDTBL_LOW     (1ULL << 13)
#define IOAPIC_REDTBL_LOGICAL (1ULL << 11)

static volatile uint32_t *ioapic_base = 0;
static uint32_t ioapic_entries = 0; /* Number of redirect entries */
static int ioapic_ready = 0;

static inline uint32_t ioapic_read(uint32_t reg)
{
  ioapic_base[0] = reg;
  return ioapic_base[4];
}

static inline void ioapic_write(uint32_t reg, uint32_t val)
{
  ioapic_base[0] = reg;
  ioapic_base[4] = val;
}

int brights_ioapic_init(uint64_t mmio_addr)
{
  ioapic_base = (volatile uint32_t *)(uintptr_t)mmio_addr;

  /* Read version to verify it's valid */
  uint32_t ver = ioapic_read(IOAPIC_REG_VER);
  ioapic_entries = ((ver >> 16) & 0xFF) + 1;

  if (ioapic_entries == 0 || ioapic_entries > 256) {
    ioapic_base = 0;
    return -1;
  }

  /* Mask all entries */
  for (uint32_t i = 0; i < ioapic_entries; ++i) {
    uint32_t lo_off = IOAPIC_REG_REDTBL + i * 2;
    uint32_t hi_off = lo_off + 1;
    ioapic_write(hi_off, 0);
    ioapic_write(lo_off, (uint32_t)IOAPIC_REDTBL_MASKED);
  }

  ioapic_ready = 1;
  return 0;
}

void brights_ioapic_set_redirect(uint32_t gsi, uint8_t vector,
                                  uint8_t delivery, uint32_t dest_apic_id)
{
  if (!ioapic_ready || gsi >= ioapic_entries) return;

  uint32_t lo_off = IOAPIC_REG_REDTBL + gsi * 2;
  uint32_t hi_off = lo_off + 1;

  uint64_t entry = vector;
  entry |= ((uint64_t)delivery & 0x7) << 8;
  /* Destination mode: physical (bit 11 = 0) */
  /* Pin polarity: high active (bit 13 = 0) */
  /* Trigger mode: edge (bit 15 = 0) */

  ioapic_write(hi_off, dest_apic_id << 24);
  ioapic_write(lo_off, (uint32_t)(entry & 0xFFFFFFFF));
}

void brights_ioapic_mask(uint32_t gsi)
{
  if (!ioapic_ready || gsi >= ioapic_entries) return;
  uint32_t lo_off = IOAPIC_REG_REDTBL + gsi * 2;
  uint32_t val = ioapic_read(lo_off);
  val |= (uint32_t)IOAPIC_REDTBL_MASKED;
  ioapic_write(lo_off, val);
}

void brights_ioapic_unmask(uint32_t gsi)
{
  if (!ioapic_ready || gsi >= ioapic_entries) return;
  uint32_t lo_off = IOAPIC_REG_REDTBL + gsi * 2;
  uint32_t val = ioapic_read(lo_off);
  val &= ~(uint32_t)IOAPIC_REDTBL_MASKED;
  ioapic_write(lo_off, val);
}

uint32_t brights_ioapic_gsi_count(void)
{
  return ioapic_entries;
}
