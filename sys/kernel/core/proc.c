#include <stdint.h>

#define BRIGHTS_PROC_MAX 64

typedef enum {
  BRIGHTS_PROC_UNUSED = 0,
  BRIGHTS_PROC_RUNNABLE = 1,
  BRIGHTS_PROC_RUNNING = 2,
  BRIGHTS_PROC_SLEEPING = 3,
} brights_proc_state_t;

typedef struct {
  uint32_t pid;
  brights_proc_state_t state;
} brights_proc_t;

static brights_proc_t proc_table[BRIGHTS_PROC_MAX];
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
