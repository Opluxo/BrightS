#include <stdint.h>

static uint64_t sched_ticks;
static uint64_t sched_dispatches;

void brights_sched_init(void)
{
  sched_ticks = 0;
  sched_dispatches = 0;
}

void brights_sched_tick(void)
{
  ++sched_ticks;
}

uint64_t brights_sched_ticks(void)
{
  return sched_ticks;
}

uint64_t brights_sched_dispatches(void)
{
  return sched_dispatches;
}

int brights_sched_mark_dispatch(void)
{
  ++sched_dispatches;
  return 0;
}
