#ifndef BRIGHTS_SCHED_H
#define BRIGHTS_SCHED_H

#include <stdint.h>
#include "proc.h"

void brights_sched_init(void);
void brights_sched_tick(void);
uint64_t brights_sched_ticks(void);
uint64_t brights_sched_dispatches(void);
int brights_sched_mark_dispatch(void);

int brights_sched_add_process(uint32_t pid);
int brights_sched_remove_process(uint32_t pid);
int brights_sched_schedule(void);
uint32_t brights_sched_current_pid(void);
int brights_sched_yield(void);

/* Set the trap frame for context switch during timer interrupt */
void brights_sched_set_trap_frame(void *tf);

/* Get the trap frame of the current process (for context switch) */
void *brights_sched_get_trap_frame(void);

#endif
