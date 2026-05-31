#include "sleep.h"
#include "clock.h"
#include "hwinfo.h"
#ifdef __i386__
/* HPET not available on i386 minimal port */
#else
#include "../arch/x86_64/hpet.h"
#endif
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
#ifndef __i386__
  brights_hpet_nsleep(us * 1000);
#else
  (void)us;
  for (volatile uint64_t i = 0; i < us * 100; ++i) { __asm__ __volatile__("pause"); }
#endif
}

void brights_sleep_ms(uint64_t ms)
{
#ifndef __i386__
  brights_hpet_nsleep(ms * 1000000);
#else
  if (ms == 0) return;
  uint64_t ticks = (ms + 9) / 10;
  if (ticks == 0) ticks = 1;
  uint64_t start = brights_clock_now_ticks();
  while (brights_clock_now_ticks() - start < ticks) {
    __asm__ __volatile__("hlt");
  }
#endif
}

void brights_halt(void)
{
  __asm__ __volatile__("hlt" ::: "memory");
}

uint64_t brights_sleep_counter(void)
{
  return sleep_spin_counter;
}
