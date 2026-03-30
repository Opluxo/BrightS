#include <stdint.h>
#include "proc.h"

#define BRIGHTS_SCHED_MAX_PROC 64u

static uint64_t sched_ticks;
static uint64_t sched_dispatches;

// Simple round-robin scheduler
static uint32_t run_queue[BRIGHTS_SCHED_MAX_PROC];
static uint32_t run_queue_count = 0;
static uint32_t current_index = 0;
static uint32_t current_pid = 0;

void brights_sched_init(void)
{
  sched_ticks = 0;
  sched_dispatches = 0;
  run_queue_count = 0;
  current_index = 0;
  current_pid = 0;
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

int brights_sched_add_process(uint32_t pid)
{
  if (pid == 0 || run_queue_count >= BRIGHTS_SCHED_MAX_PROC) {
    return -1;
  }
  
  // Check if already in queue
  for (uint32_t i = 0; i < run_queue_count; ++i) {
    if (run_queue[i] == pid) {
      return 0; // Already in queue
    }
  }
  
  run_queue[run_queue_count++] = pid;
  return 0;
}

int brights_sched_remove_process(uint32_t pid)
{
  if (pid == 0) {
    return -1;
  }
  
  for (uint32_t i = 0; i < run_queue_count; ++i) {
    if (run_queue[i] == pid) {
      // Shift remaining elements
      for (uint32_t j = i; j < run_queue_count - 1; ++j) {
        run_queue[j] = run_queue[j + 1];
      }
      --run_queue_count;
      
      // Adjust current index if needed
      if (current_index >= run_queue_count && run_queue_count > 0) {
        current_index = 0;
      }
      
      return 0;
    }
  }
  
  return -1; // Not found
}

int brights_sched_schedule(void)
{
  if (run_queue_count == 0) {
    current_pid = 0;
    return -1;
  }
  
  // Find next runnable process
  uint32_t attempts = 0;
  while (attempts < run_queue_count) {
    uint32_t pid = run_queue[current_index];
    brights_proc_state_t state = BRIGHTS_PROC_UNUSED;
    
    // Get process state
    for (uint32_t i = 0; i < BRIGHTS_SCHED_MAX_PROC; ++i) {
      brights_proc_info_t info;
      if (brights_proc_info_at(i, &info) == 0 && info.pid == pid) {
        state = info.state;
        break;
      }
    }
    
    if (state == BRIGHTS_PROC_RUNNABLE || state == BRIGHTS_PROC_RUNNING) {
      current_pid = pid;
      brights_proc_set_state(pid, BRIGHTS_PROC_RUNNING);
      brights_sched_mark_dispatch();
      return 0;
    }
    
    current_index = (current_index + 1) % run_queue_count;
    ++attempts;
  }
  
  current_pid = 0;
  return -1;
}

uint32_t brights_sched_current_pid(void)
{
  return current_pid;
}

int brights_sched_yield(void)
{
  if (current_pid == 0) {
    return -1;
  }
  
  // Mark current as runnable
  brights_proc_set_state(current_pid, BRIGHTS_PROC_RUNNABLE);
  
  // Move to next process
  current_index = (current_index + 1) % run_queue_count;
  return brights_sched_schedule();
}
