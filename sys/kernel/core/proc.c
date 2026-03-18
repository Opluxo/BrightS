#include "proc.h"

#define BRIGHTS_PROC_MAX 64u

static brights_proc_info_t proc_table[BRIGHTS_PROC_MAX];
static uint32_t next_pid = 1;

void brights_proc_init(void)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    proc_table[i].pid = 0;
    proc_table[i].state = BRIGHTS_PROC_UNUSED;
  }
  next_pid = 1;
}

int brights_proc_spawn_kernel(void)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == BRIGHTS_PROC_UNUSED) {
      proc_table[i].pid = next_pid++;
      proc_table[i].state = BRIGHTS_PROC_RUNNABLE;
      return (int)proc_table[i].pid;
    }
  }
  return -1;
}

int brights_proc_set_state(uint32_t pid, brights_proc_state_t state)
{
  if (pid == 0 || state == BRIGHTS_PROC_UNUSED) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      proc_table[i].state = state;
      return 0;
    }
  }
  return -1;
}

uint32_t brights_proc_count(brights_proc_state_t state)
{
  uint32_t count = 0;
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == state) {
      ++count;
    }
  }
  return count;
}

uint32_t brights_proc_total(void)
{
  uint32_t count = 0;
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      ++count;
    }
  }
  return count;
}

int brights_proc_info_at(uint32_t index, brights_proc_info_t *info_out)
{
  if (!info_out || index >= BRIGHTS_PROC_MAX) {
    return -1;
  }
  *info_out = proc_table[index];
  return (info_out->state == BRIGHTS_PROC_UNUSED) ? -1 : 0;
}
