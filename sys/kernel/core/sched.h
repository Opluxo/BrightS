#ifndef BRIGHTS_SCHED_H
#define BRIGHTS_SCHED_H

#include <stdint.h>
#include "proc.h"

void brights_sched_init(void);
void brights_sched_tick(void);
uint64_t brights_sched_ticks(void);
uint64_t brights_sched_dispatches(void);
int brights_sched_mark_dispatch(void);

// New scheduler functions
int brights_sched_add_process(uint32_t pid);
int brights_sched_remove_process(uint32_t pid);
int brights_sched_schedule(void);
uint32_t brights_sched_current_pid(void);
int brights_sched_yield(void);

#endif
