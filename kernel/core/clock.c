#include "clock.h"
#include "hwinfo.h"
#include <stdint.h>

static uint64_t clock_ticks;
static uint64_t tsc_freq = 0; // TSC frequency in Hz

// Read TSC (Time Stamp Counter)
static inline uint64_t rdtsc(void)
{
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

// Read TSC with serialization (RDTSCP) - more accurate
static inline uint64_t rdtscp(void)
{
  uint32_t lo, hi, aux;
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
  (void)aux;
  return ((uint64_t)hi << 32) | lo;
}

// Check if TSC is available
static int has_tsc(void)
{
  uint32_t eax, ebx, ecx, edx;
  __asm__ __volatile__("cpuid"
                        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                        : "a"(1));
  return (edx >> 4) & 1; // TSC bit
}

void brights_clock_init(void)
{
  clock_ticks = 0;
  tsc_freq = 0;
}

void brights_clock_tick(void)
{
  ++clock_ticks;
}

uint64_t brights_clock_now_ticks(void)
{
  return clock_ticks;
}

void brights_clock_advance(uint64_t ticks)
{
  clock_ticks += ticks;
}

void brights_clock_calibrate(void)
{
  if (tsc_freq > 0) return; /* Already calibrated */
  if (!has_tsc()) return;

  /* Use hwinfo calibration if available */
  const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
  if (cpu && cpu->tsc_freq > 0) {
    tsc_freq = cpu->tsc_freq;
    return;
  }

  /* Fallback: 2.4 GHz default for modern CPUs */
  tsc_freq = 2400000000ULL;
}

uint64_t brights_clock_ns(void)
{
  if (tsc_freq == 0) {
    brights_clock_calibrate();
  }

  if (tsc_freq > 0 && has_tsc()) {
    /* Use RDTSCP for serialized, more accurate reading */
    uint64_t tsc = rdtscp();
    /* Convert TSC ticks to nanoseconds using fixed-point math */
    /* ns = (tsc * 1000000000) / freq */
    /* Use 128-bit intermediate to avoid overflow: split the division */
    uint64_t freq_mhz = tsc_freq / 1000000;
    if (freq_mhz > 0) {
      /* tsc / freq_mhz gives microseconds, * 1000 gives nanoseconds */
      return (tsc / freq_mhz) * 1000;
    }
    /* Fallback for very low frequencies */
    return (tsc / (tsc_freq / 1000000000ULL));
  }

  /* Fallback: estimate from ticks (assuming 100Hz timer) */
  return clock_ticks * 10000000ULL; /* 10ms per tick */
}

uint64_t brights_clock_us(void)
{
  return brights_clock_ns() / 1000;
}

uint64_t brights_clock_ms(void)
{
  return brights_clock_ns() / 1000000;
}

uint64_t brights_clock_tsc_freq(void)
{
  if (tsc_freq == 0) {
    brights_clock_calibrate();
  }
  return tsc_freq;
}
