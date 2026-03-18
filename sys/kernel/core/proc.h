#ifndef BRIGHTS_PROC_H
#define BRIGHTS_PROC_H

#include <stdint.h>

typedef enum {
  BRIGHTS_PROC_UNUSED = 0,
  BRIGHTS_PROC_RUNNABLE = 1,
  BRIGHTS_PROC_RUNNING = 2,
  BRIGHTS_PROC_SLEEPING = 3,
} brights_proc_state_t;

typedef struct {
  uint32_t pid;
  brights_proc_state_t state;
} brights_proc_info_t;

void brights_proc_init(void);
int brights_proc_spawn_kernel(void);
int brights_proc_set_state(uint32_t pid, brights_proc_state_t state);
uint32_t brights_proc_count(brights_proc_state_t state);
uint32_t brights_proc_total(void);
int brights_proc_info_at(uint32_t index, brights_proc_info_t *info_out);

#endif
