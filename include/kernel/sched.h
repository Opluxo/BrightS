#ifndef BRIGHTS_SCHED_H
#define BRIGHTS_SCHED_H

#include <stdint.h>
#include "proc.h"

/* === Core scheduler API === */
void     brights_sched_init(void);
void     brights_sched_tick(void);
uint64_t brights_sched_ticks(void);
uint64_t brights_sched_dispatches(void);
int      brights_sched_mark_dispatch(void);

/* === Process lifecycle === */
int      brights_sched_add_process(uint32_t pid);
int      brights_sched_remove_process(uint32_t pid);
int      brights_sched_schedule(void);

/* === Query === */
uint32_t brights_sched_current_pid(void);

/* === Context switch === */
void     brights_sched_set_trap_frame(void *tf);
void    *brights_sched_get_trap_frame(void);
int      brights_sched_yield(void);

/* === Priority / resource control === */
int      brights_sched_set_nice(uint32_t pid, int32_t nice);
int      brights_sched_get_stats(uint32_t pid, brights_proc_sched_t *out);

#endif
