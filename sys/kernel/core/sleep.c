#include <stdint.h>

static volatile uint64_t sleep_spin_counter;

// Approximate CPU frequency for timing (will be calibrated)
static uint64_t cpu_freq_mhz = 2400; // Default assumption: 2.4 GHz

void brights_sleep_cycles(uint64_t cycles)
{
  // Use pause instruction for better power efficiency on modern CPUs
  for (uint64_t i = 0; i < cycles; ++i) {
    __asm__ __volatile__("pause" ::: "memory");
  }
  sleep_spin_counter += cycles;
}

void brights_sleep_us(uint64_t us)
{
  // Calculate cycles needed (approximate)
  // cycles = us * (freq_mhz / 1000)
  uint64_t cycles = (us * cpu_freq_mhz) / 1000;
  if (cycles == 0) {
    cycles = 1;
  }
  brights_sleep_cycles(cycles);
}

void brights_sleep_ms(uint64_t ms)
{
  brights_sleep_us(ms * 1000);
}

void brights_halt(void)
{
  // Halt until next interrupt - most power efficient
  __asm__ __volatile__("hlt" ::: "memory");
}
