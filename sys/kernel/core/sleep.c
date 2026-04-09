#include "sleep.h"
#include "hwinfo.h"
#include "../platform/x86_64/hpet.h"
#include <stdint.h>

static volatile uint64_t sleep_spin_counter;
void brights_sleep_cycles(uint64_t cycles)
{
  for (uint64_t i = 0; i < cycles; ++i) {
    __asm__ __volatile__("pause" ::: "memory");
  }
  sleep_spin_counter += cycles;
}

void brights_sleep_us(uint64_t us)
{
  /* Use HPET if available (precise) */
  brights_hpet_nsleep(us * 1000);
}

void brights_sleep_ms(uint64_t ms)
{
  /* Use HPET for precise millisecond delays */
  brights_hpet_nsleep(ms * 1000000);
}

void brights_halt(void)
{
  __asm__ __volatile__("hlt" ::: "memory");
}

uint64_t brights_sleep_counter(void)
{
  return sleep_spin_counter;
}
