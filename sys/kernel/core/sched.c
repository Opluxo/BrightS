#include <stdint.h>

static uint64_t sched_ticks;

void brights_sched_init(void)
{
  sched_ticks = 0;
}

void brights_sched_tick(void)
{
  ++sched_ticks;
}

uint64_t brights_sched_ticks(void)
{
  return sched_ticks;
}
