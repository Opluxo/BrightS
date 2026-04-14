#ifndef BRIGHTS_PROC_H
#define BRIGHTS_PROC_H

#include <stdint.h>
#include "fs/vfs2.h"
#include "elf.h"

#define BRIGHTS_PROC_NAME_LEN 32
#define BRIGHTS_PROC_CWD_LEN 128
#define BRIGHTS_PROC_MAX_FDS VFS_MAX_FDS

/* Kernel stack per process: 8KB */
#define BRIGHTS_PROC_KSTACK_SIZE 8192

typedef enum {
  BRIGHTS_PROC_UNUSED = 0,
  BRIGHTS_PROC_RUNNABLE = 1,
  BRIGHTS_PROC_RUNNING = 2,
  BRIGHTS_PROC_SLEEPING = 3,
  BRIGHTS_PROC_ZOMBIE = 4,
} brights_proc_state_t;

/* Saved register context for context switching */
typedef struct {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rip, cs, rflags, rsp, ss;
} brights_proc_ctx_t;

typedef struct {
  uint32_t pid;
  uint32_t ppid;  // Parent process ID
  brights_proc_state_t state;
  char name[BRIGHTS_PROC_NAME_LEN];
  int exit_code;
  vfs_file_t *fd_table[BRIGHTS_PROC_MAX_FDS];
  char cwd[BRIGHTS_PROC_CWD_LEN];

  /* User-mode context */
  int is_user;                    /* 1 if user-mode process */
  uint64_t cr3;                   /* Per-process page table (physical address) */
  uint64_t user_entry;            /* User entry point */
  uint64_t user_stack_top;        /* Top of user stack */
  uint64_t brk;                   /* Program break (heap end) */
  uint64_t brk_start;             /* Initial brk value */
  uint64_t kernel_stack;          /* Kernel stack top for this process */
  brights_proc_ctx_t ctx;         /* Saved context */
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

// Get fd_table of current process
vfs_file_t **brights_proc_fd_table(void);

// Get/set cwd of current process
const char *brights_proc_get_cwd(void);
int brights_proc_set_cwd(const char *path);

// Resolve a path relative to current cwd
int brights_proc_resolve_path(const char *path, char *out, int cap);

// Spawn a user-mode process with given entry point and stack
int brights_proc_spawn_user(const char *name, uint64_t entry, uint64_t stack_top);

// Get CR3 (page table) for a process
uint64_t brights_proc_get_cr3(uint32_t pid);

// Set process brk
int brights_proc_set_brk(uint32_t pid, uint64_t brk);

// Get process brk
uint64_t brights_proc_get_brk(uint32_t pid);

// Get kernel stack top for current process
uint64_t brights_proc_kernel_stack(void);

// Fork: create a child process
int brights_proc_fork(void);

// Exec: replace current process with new program
int brights_proc_exec(const void *elf_data, uint64_t elf_size, elf64_load_info_t *info_out);

// Continue exec: enter new user program (does not return)
int brights_proc_exec_continue(void);

// Get pointer to process table (for scheduler)
brights_proc_info_t *brights_proc_table_ptr(void);

#endif
