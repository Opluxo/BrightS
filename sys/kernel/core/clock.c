#include <stdint.h>

static uint64_t clock_ticks;

void brights_clock_init(void)
{
  clock_ticks = 0;
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
