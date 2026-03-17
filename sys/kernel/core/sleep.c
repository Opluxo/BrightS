#include <stdint.h>

static volatile uint64_t sleep_spin_counter;

void brights_sleep_cycles(uint64_t cycles)
{
  for (uint64_t i = 0; i < cycles; ++i) {
    ++sleep_spin_counter;
    __asm__ __volatile__("pause");
  }
}
