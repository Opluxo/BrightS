#include "hpet.h"
#include <stdint.h>

/* HPET MMIO register offsets */
#define HPET_REG_CAP_ID    0x000  /* General Capabilities and ID */
#define HPET_REG_CONFIG    0x010  /* General Configuration */
#define HPET_REG_STATUS    0x020  /* General Interrupt Status */
#define HPET_REG_COUNTER   0x0F0  /* Main Counter Value */

/* Capability register bits */
#define HPET_CAP_COUNT_SIZE (1ULL << 13)  /* 64-bit counter */
#define HPET_CAP_LEGACY_RT (1ULL << 15)   /* Legacy replacement capable */

/* Config register bits */
#define HPET_CONFIG_ENABLE  (1ULL << 0)   /* Overall enable */
#define HPET_CONFIG_LEGACY  (1ULL << 1)   /* Legacy replacement route */

static volatile uint64_t *hpet_base = 0;
static uint32_t hpet_period_fs = 0; /* Tick period in femtoseconds */
static uint64_t hpet_freq = 0;
static int hpet_ready = 0;

int brights_hpet_init(uint64_t mmio_addr)
{
  hpet_base = (volatile uint64_t *)(uintptr_t)mmio_addr;
  if (!hpet_base) return -1;

  /* Read capabilities */
  uint64_t cap = hpet_base[HPET_REG_CAP_ID / 8];
  hpet_period_fs = (uint32_t)((cap >> 32) & 0xFFFFFFFF);

  if (hpet_period_fs == 0 || hpet_period_fs > 100000000) {
    /* Invalid period */
    hpet_base = 0;
    return -1;
  }

  /* Calculate frequency: 10^15 / period_fs */
  hpet_freq = 1000000000000000ULL / hpet_period_fs;

  /* Enable HPET: set enable bit, disable legacy routing */
  hpet_base[HPET_REG_CONFIG / 8] = HPET_CONFIG_ENABLE;

  /* Reset counter to 0 */
  hpet_base[HPET_REG_COUNTER / 8] = 0;

  hpet_ready = 1;
  return 0;
}

uint64_t brights_hpet_read_counter(void)
{
  if (!hpet_ready || !hpet_base) return 0;
  return hpet_base[HPET_REG_COUNTER / 8];
}

uint32_t brights_hpet_period_fs(void)
{
  return hpet_period_fs;
}

uint64_t brights_hpet_freq(void)
{
  return hpet_freq;
}

void brights_hpet_nsleep(uint64_t ns)
{
  if (!hpet_ready || !hpet_base) return;

  /* Convert ns to HPET ticks: ticks = ns * 10^6 / period_fs */
  /* To avoid overflow: ticks = ns * (1000000 / period_fs) * 1000 */
  uint64_t ticks;
  if (hpet_period_fs <= 1000000) {
    ticks = ns * (1000000ULL / hpet_period_fs);
  } else {
    ticks = ns / (hpet_period_fs / 1000000ULL);
  }

  uint64_t start = hpet_base[HPET_REG_COUNTER / 8];
  while (hpet_base[HPET_REG_COUNTER / 8] - start < ticks) {
    __asm__ __volatile__("pause");
  }
}
