#ifndef BRIGHTS_PROC_H
#define BRIGHTS_PROC_H

#include <stdint.h>

#define BRIGHTS_PROC_NAME_LEN 32

typedef enum {
  BRIGHTS_PROC_UNUSED = 0,
  BRIGHTS_PROC_RUNNABLE = 1,
  BRIGHTS_PROC_RUNNING = 2,
  BRIGHTS_PROC_SLEEPING = 3,
  BRIGHTS_PROC_ZOMBIE = 4,
} brights_proc_state_t;

typedef struct {
  uint32_t pid;
  uint32_t ppid;  // Parent process ID
  brights_proc_state_t state;
  char name[BRIGHTS_PROC_NAME_LEN];
  int exit_code;
} brights_proc_info_t;

// Initialize process subsystem
void brights_proc_init(void);

// Spawn a new kernel process
int brights_proc_spawn_kernel(const char *name);

// Set process state
int brights_proc_set_state(uint32_t pid, brights_proc_state_t state);

// Count processes in given state
uint32_t brights_proc_count(brights_proc_state_t state);

// Get total number of processes
uint32_t brights_proc_total(void);

// Get process info at index
int brights_proc_info_at(uint32_t index, brights_proc_info_t *info_out);

// Get process info by PID
int brights_proc_get_by_pid(uint32_t pid, brights_proc_info_t *info_out);

// Terminate a process
int brights_proc_exit(uint32_t pid, int exit_code);

// Get current process PID
uint32_t brights_proc_current(void);

// Set current process PID
void brights_proc_set_current(uint32_t pid);

#endif
