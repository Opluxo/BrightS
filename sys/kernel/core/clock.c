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
  if (!has_tsc()) {
    return;
  }

  // Simple calibration: count TSC ticks over a known period
  // We'll use a fixed estimate for now (can be improved with PIT/APIC timer)
  // Typical modern CPUs: 2-4 GHz
  tsc_freq = 2400000000ULL; // 2.4 GHz default
}

uint64_t brights_clock_ns(void)
{
  if (tsc_freq == 0) {
    brights_clock_calibrate();
  }

  if (tsc_freq > 0 && has_tsc()) {
    uint64_t tsc = rdtsc();
    // Convert TSC ticks to nanoseconds
    // ns = (tsc * 1000000000) / freq
    // To avoid overflow, divide first if freq is large
    if (tsc_freq >= 1000000000ULL) {
      return (tsc / (tsc_freq / 1000000000ULL));
    } else {
      return (tsc * 1000000000ULL) / tsc_freq;
    }
  }

  // Fallback: estimate from ticks (assuming 100Hz timer)
  return clock_ticks * 10000000ULL; // 10ms per tick
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
