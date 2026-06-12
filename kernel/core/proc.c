#include "proc.h"
#include "sched.h"
#include "pmem.h"
#include "syshook.h"
#include "kernel_util.h"
#include "elf.h"
#ifdef __i386__
#include "../arch/i386/paging.h"
#else
#include "../arch/x86_64/paging.h"
#endif
#include "../drivers/serial.h"

#define BRIGHTS_PROC_MAX 64u
#define BRIGHTS_PATH_MAX_SEG 16u
#define BRIGHTS_PATH_SEG_LEN 64u
#define BRIGHTS_PATH_COMBINED_LEN 256u

static brights_proc_info_t proc_table[BRIGHTS_PROC_MAX];
static uint32_t pid_to_index[256];
static uint64_t free_slots_bitmap;
static uint64_t pid_bitmap;       /* O(1) PID allocation */
static uint32_t next_pid = 1;
static uint32_t current_pid = 0;

static uint32_t proc_alloc_pid(void)
{
  /* O(1) PID allocation using bitmap + BSF */
  uint64_t avail = ~pid_bitmap & 0xFFFFFFFFULL; /* PIDs 1-31 */
  if (avail == 0) return 0;
  uint64_t pid64;
  __asm__ __volatile__("bsf %1, %0" : "=r"(pid64) : "r"(avail) : "cc");
  uint32_t pid = (uint32_t)pid64;
  if (pid == 0 || pid >= 256) return 0;
  pid_bitmap |= (1ULL << pid);
  return pid;
}

brights_proc_info_t *brights_proc_table_ptr(void)
{
  return proc_table;
}

uint32_t brights_proc_index(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return BRIGHTS_PROC_MAX;
  return pid_to_index[pid];
}

static inline void proc_str_copy(char *dst, int cap, const char *src)
{
  if (!dst || cap <= 0) return;
  int i = 0;
  while (src && src[i] && i < cap - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
}

static inline brights_proc_info_t *proc_get_by_pid(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return 0;
  uint32_t idx = pid_to_index[pid];
  if (idx >= BRIGHTS_PROC_MAX) return 0;
  if (proc_table[idx].pid != pid || proc_table[idx].state == BRIGHTS_PROC_UNUSED) return 0;
  return &proc_table[idx];
}

static inline brights_proc_info_t *proc_get_current(void)
{
  return proc_get_by_pid(current_pid);
}

static inline int proc_find_free_slot(void)
{
  uint64_t bitmap = ~free_slots_bitmap;
  if (bitmap == 0) return -1;
  return (int)__builtin_ctzll(bitmap);
}

static inline void proc_mark_slot_used(int slot)
{
  free_slots_bitmap |= (1ULL << slot);
}

static inline void proc_free_slot(int slot)
{
  if (slot < 0 || slot >= (int)BRIGHTS_PROC_MAX) return;
  free_slots_bitmap &= ~(1ULL << slot);
}

static inline void proc_sched_zero(brights_proc_sched_t *s)
{
  s->cpu_ticks = 0;
  s->sched_count = 0;
  s->last_sched_tick = 0;
  s->total_wait_ticks = 0;
  s->priority = BRIGHTS_SCHED_PRIO_NORMAL;
  s->nice = 0;
  s->boost_scheduled = 0;
}

void brights_proc_init(void)
{
  free_slots_bitmap = 0;
  pid_bitmap = (1ULL << 0); /* PID 0 is reserved */
  for (uint32_t i = 0; i < 256; ++i) pid_to_index[i] = BRIGHTS_PROC_MAX;

  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    proc_table[i].pid = 0;
    proc_table[i].ppid = 0;
    proc_table[i].state = BRIGHTS_PROC_UNUSED;
    proc_table[i].name[0] = 0;
    proc_table[i].exit_code = 0;
    for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
      proc_table[i].fd_table[j] = 0;
    }
    proc_table[i].cwd[0] = '/';
    proc_table[i].cwd[1] = 0;
    proc_table[i].is_user = 0;
    proc_table[i].cr3 = 0;
    proc_table[i].user_entry = 0;
    proc_table[i].user_stack_top = 0;
    proc_table[i].brk = 0;
    proc_table[i].brk_start = 0;
    proc_table[i].kernel_stack = 0;
    proc_sched_zero(&proc_table[i].sched);
    brights_signal_proc_init(&proc_table[i].signal);
    proc_table[i].env_count = 0;
  }
  next_pid = 1;
  current_pid = 0;
}

int brights_proc_spawn_kernel(const char *name)
{
  int slot = proc_find_free_slot();
  if (slot < 0) return -1;

  proc_table[slot].pid = proc_alloc_pid();
  proc_table[slot].ppid = current_pid;
  proc_table[slot].state = BRIGHTS_PROC_RUNNABLE;
  proc_table[slot].exit_code = 0;
  proc_table[slot].is_user = 0;
  proc_table[slot].cr3 = 0;
  proc_table[slot].user_entry = 0;
  proc_table[slot].user_stack_top = 0;
  proc_table[slot].brk = 0;
  proc_table[slot].brk_start = 0;
  proc_table[slot].kernel_stack = 0;
  proc_sched_zero(&proc_table[slot].sched);
  proc_str_copy(proc_table[slot].name, BRIGHTS_PROC_NAME_LEN, name ? name : "kernel");

  for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
    proc_table[slot].fd_table[j] = 0;
  }

  brights_proc_info_t *parent = proc_get_current();
  if (parent) {
    proc_str_copy(proc_table[slot].cwd, BRIGHTS_PROC_CWD_LEN, parent->cwd);
  } else {
    proc_table[slot].cwd[0] = '/';
    proc_table[slot].cwd[1] = 0;
  }

  pid_to_index[proc_table[slot].pid] = (uint32_t)slot;
  proc_mark_slot_used(slot);

  return (int)proc_table[slot].pid;
}

int brights_proc_set_state(uint32_t pid, brights_proc_state_t state)
{
  brights_proc_info_t *p = proc_get_by_pid(pid);
  if (!p) return -1;
  p->state = state;
  return 0;
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

int brights_proc_get_by_pid(uint32_t pid, brights_proc_info_t *info_out)
{
  if (!info_out || pid == 0 || pid >= 256) return -1;
  uint32_t idx = pid_to_index[pid];
  if (idx >= BRIGHTS_PROC_MAX) return -1;
  if (proc_table[idx].pid != pid || proc_table[idx].state == BRIGHTS_PROC_UNUSED) return -1;
  *info_out = proc_table[idx];
  return 0;
}

int brights_proc_exit(uint32_t pid, int exit_code)
{
  brights_proc_info_t *p = proc_get_by_pid(pid);
  if (!p) return -1;

  for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
    if (p->fd_table[j]) {
      brights_vfs2_close(p->fd_table[j]);
      p->fd_table[j] = 0;
    }
  }

  brights_syshook_cleanup_pid(pid);

  if (p->cr3 != 0 && p->is_user) {
    uint64_t kernel_cr3 = brights_paging_get_cr3();
    if (p->cr3 != kernel_cr3) {
      brights_paging_destroy_address_space(p->cr3);
    }
  }

  if (p->kernel_stack != 0) {
    void *kstack_page = (void *)(uintptr_t)(p->kernel_stack - BRIGHTS_PROC_KSTACK_SIZE);
    brights_pmem_free_page(kstack_page);
  }

  p->state = BRIGHTS_PROC_ZOMBIE;
  p->exit_code = exit_code;
  return 0;
}

int brights_proc_reap(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return -1;
  uint32_t idx = pid_to_index[pid];
  if (idx >= BRIGHTS_PROC_MAX) return -1;
  if (proc_table[idx].pid != pid || proc_table[idx].state != BRIGHTS_PROC_ZOMBIE) return -1;
  /* Remove from scheduler first */
  brights_sched_remove_process(pid);
  /* Free proc table slot */
  pid_to_index[pid] = BRIGHTS_PROC_MAX;
  proc_free_slot((int)idx);
  /* Free PID in bitmap */
  pid_bitmap &= ~(1ULL << pid);
  return 0;
}

uint32_t brights_proc_current(void)
{
  return current_pid;
}

void brights_proc_set_current(uint32_t pid)
{
  current_pid = pid;
}

/* ---- fd_table operations ---- */
vfs_file_t **brights_proc_fd_table(void)
{
  brights_proc_info_t *p = proc_get_current();
  if (!p) return 0;
  return p->fd_table;
}

/* ---- cwd operations ---- */
const char *brights_proc_get_cwd(void)
{
  brights_proc_info_t *p = proc_get_current();
  if (!p) return "/";
  return p->cwd;
}

int brights_proc_set_cwd(const char *path)
{
  if (!path) return -1;
  brights_proc_info_t *p = proc_get_current();
  if (!p) return -1;
  proc_str_copy(p->cwd, BRIGHTS_PROC_CWD_LEN, path);
  return 0;
}

/* ---- Path resolution: resolve relative path against cwd ---- */
int brights_proc_resolve_path(const char *path, char *out, int cap)
{
  if (!path || !out || cap < 2) return -1;

  const char *cwd = brights_proc_get_cwd();

  if (path[0] == '/') {
    char segs[BRIGHTS_PATH_MAX_SEG][BRIGHTS_PATH_SEG_LEN];
    int seg_count = 0;
    int i = 0;
    while (path[i]) {
      while (path[i] == '/') ++i;
      if (!path[i]) break;
      char seg[BRIGHTS_PATH_SEG_LEN];
      int slen = 0;
      while (path[i] && path[i] != '/') {
        if (slen >= (int)BRIGHTS_PATH_SEG_LEN - 1) return -1;
        seg[slen++] = path[i++];
      }
      seg[slen] = 0;
      if (slen == 1 && seg[0] == '.') continue;
      if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
        if (seg_count > 0) --seg_count;
        continue;
      }
      if (seg_count >= (int)BRIGHTS_PATH_MAX_SEG) return -1;
      for (int j = 0; j <= slen; ++j) segs[seg_count][j] = seg[j];
      ++seg_count;
    }
    int p = 0;
    out[p++] = '/';
    for (int s = 0; s < seg_count; ++s) {
      for (int j = 0; segs[s][j]; ++j) {
        if (p >= cap - 1) return -1;
        out[p++] = segs[s][j];
      }
      if (s + 1 < seg_count) {
        if (p >= cap - 1) return -1;
        out[p++] = '/';
      }
    }
    out[p] = 0;
    return 0;
  }

  char combined[BRIGHTS_PATH_COMBINED_LEN];
  int ci = 0;

  for (int i = 0; cwd[i] && ci < (int)BRIGHTS_PATH_COMBINED_LEN - 2; ++i) combined[ci++] = cwd[i];

  if (ci > 0 && combined[ci - 1] != '/' && path[0] != 0) {
    if (ci < (int)BRIGHTS_PATH_COMBINED_LEN - 2) combined[ci++] = '/';
  }

  for (int i = 0; path[i] && ci < (int)BRIGHTS_PATH_COMBINED_LEN - 2; ++i) combined[ci++] = path[i];
  combined[ci] = 0;

  char segs[BRIGHTS_PATH_MAX_SEG][BRIGHTS_PATH_SEG_LEN];
  int seg_count = 0;
  int i = 0;
  while (combined[i]) {
    while (combined[i] == '/') ++i;
    if (!combined[i]) break;
    char seg[BRIGHTS_PATH_SEG_LEN];
    int slen = 0;
    while (combined[i] && combined[i] != '/') {
      if (slen >= (int)BRIGHTS_PATH_SEG_LEN - 1) return -1;
      seg[slen++] = combined[i++];
    }
    seg[slen] = 0;
    if (slen == 1 && seg[0] == '.') continue;
    if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
      if (seg_count > 0) --seg_count;
      continue;
    }
    if (seg_count >= (int)BRIGHTS_PATH_MAX_SEG) return -1;
    for (int j = 0; j <= slen; ++j) segs[seg_count][j] = seg[j];
    ++seg_count;
  }

  int p = 0;
  out[p++] = '/';
  for (int s = 0; s < seg_count; ++s) {
    for (int j = 0; segs[s][j]; ++j) {
      if (p >= cap - 1) return -1;
      out[p++] = segs[s][j];
    }
    if (s + 1 < seg_count) {
      if (p >= cap - 1) return -1;
      out[p++] = '/';
    }
  }
  out[p] = 0;
  return 0;
}

/* ---- User process support ---- */

/* Allocate a new page table (PML4) with kernel mappings */
static uint64_t alloc_user_pml4(void)
{
  /* Create a new address space with kernel mappings copied from current */
  uint64_t new_cr3 = brights_paging_create_address_space();
  return new_cr3;
}

int brights_proc_spawn_user(const char *name, uint64_t entry, uint64_t stack_top)
{
  int slot = proc_find_free_slot();
  if (slot < 0) return -1;

  proc_table[slot].pid = proc_alloc_pid();
  proc_table[slot].ppid = current_pid;
  proc_table[slot].state = BRIGHTS_PROC_RUNNABLE;
  proc_table[slot].exit_code = 0;
  proc_table[slot].is_user = 1;
  proc_table[slot].user_entry = entry;
  proc_table[slot].user_stack_top = stack_top;
  proc_table[slot].brk = 0;
  proc_table[slot].brk_start = 0;
  proc_table[slot].cr3 = alloc_user_pml4();

  void *kstack_page = brights_pmem_alloc_page();
  if (kstack_page) {
    proc_table[slot].kernel_stack = (uint64_t)(uintptr_t)kstack_page + BRIGHTS_PROC_KSTACK_SIZE;
  } else {
    proc_table[slot].kernel_stack = 0;
  }

  proc_sched_zero(&proc_table[slot].sched);
  proc_str_copy(proc_table[slot].name, BRIGHTS_PROC_NAME_LEN, name ? name : "user");

  for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
    proc_table[slot].fd_table[j] = 0;
  }

  brights_proc_info_t *parent = proc_get_current();
  if (parent) {
    proc_str_copy(proc_table[slot].cwd, BRIGHTS_PROC_CWD_LEN, parent->cwd);
  } else {
    proc_table[slot].cwd[0] = '/';
    proc_table[slot].cwd[1] = 0;
  }

  proc_table[slot].ctx.rip = entry;
  proc_table[slot].ctx.rsp = stack_top;
#ifdef __i386__
  proc_table[slot].ctx.cs = 0x23;
  proc_table[slot].ctx.ss = 0x1B;
#else
  proc_table[slot].ctx.cs = 0x2B;
  proc_table[slot].ctx.ss = 0x23;
#endif
  proc_table[slot].ctx.rflags = 0x202;
  proc_table[slot].ctx.rax = 0;
  proc_table[slot].ctx.rbx = 0;
  proc_table[slot].ctx.rcx = 0;
  proc_table[slot].ctx.rdx = 0;
  proc_table[slot].ctx.rsi = 0;
  proc_table[slot].ctx.rdi = 0;
  proc_table[slot].ctx.rbp = 0;
  proc_table[slot].ctx.r8 = 0;
  proc_table[slot].ctx.r9 = 0;
  proc_table[slot].ctx.r10 = 0;
  proc_table[slot].ctx.r11 = 0;
  proc_table[slot].ctx.r12 = 0;
  proc_table[slot].ctx.r13 = 0;
  proc_table[slot].ctx.r14 = 0;
  proc_table[slot].ctx.r15 = 0;

  pid_to_index[proc_table[slot].pid] = (uint32_t)slot;
  proc_mark_slot_used(slot);

  return (int)proc_table[slot].pid;
}

uint64_t brights_proc_get_cr3(uint32_t pid)
{
  brights_proc_info_t *p = proc_get_by_pid(pid);
  if (!p) return 0;
  return p->cr3;
}

int brights_proc_set_brk(uint32_t pid, uint64_t new_brk)
{
  brights_proc_info_t *p = proc_get_by_pid(pid);
  if (!p) return -1;
  if (p->brk_start == 0) p->brk_start = new_brk;
  p->brk = new_brk;
  return 0;
}

uint64_t brights_proc_get_brk(uint32_t pid)
{
  brights_proc_info_t *p = proc_get_by_pid(pid);
  if (!p) return 0;
  return p->brk;
}

uint64_t brights_proc_kernel_stack(void)
{
  brights_proc_info_t *p = proc_get_current();
  if (!p) return 0;
  return p->kernel_stack;
}

int brights_proc_exec(const void *elf_data, uint64_t elf_size, elf64_load_info_t *info_out)
{
  brights_proc_info_t *p = proc_get_current();
  if (!p || !p->is_user) return -1;

  brights_paging_destroy_address_space(p->cr3);

  p->cr3 = brights_paging_create_address_space();
  if (p->cr3 == 0) return -1;

  if (brights_elf_load(elf_data, elf_size, info_out) != 0) {
    brights_paging_destroy_address_space(p->cr3);
    p->cr3 = brights_paging_get_cr3();
    return -1;
  }

  p->user_entry = info_out->entry;
  p->user_stack_top = info_out->stack_top;

  p->brk = 0;
  p->brk_start = 0;

  p->ctx.rip = info_out->entry;
  p->ctx.rsp = info_out->stack_top;
#ifdef __i386__
  p->ctx.cs = 0x23;
  p->ctx.ss = 0x1B;
#else
  p->ctx.cs = 0x2B;
  p->ctx.ss = 0x23;
#endif
  p->ctx.rflags = 0x202;
  p->ctx.rax = 0;
  p->ctx.rbx = 0;
  p->ctx.rcx = 0;
  p->ctx.rdx = 0;
  p->ctx.rsi = 0;
  p->ctx.rdi = 0;
  p->ctx.rbp = 0;
  p->ctx.r8 = 0;
  p->ctx.r9 = 0;
  p->ctx.r10 = 0;
  p->ctx.r11 = 0;
  p->ctx.r12 = 0;
  p->ctx.r13 = 0;
  p->ctx.r14 = 0;
  p->ctx.r15 = 0;

  return 0;
}

#ifndef __i386__
int brights_proc_exec_continue(void)
{
  brights_proc_info_t *p = proc_get_current();
  if (!p || !p->is_user) return -1;

  extern void brights_enter_user_from_syscall(void *rip, void *rsp, uint64_t cr3);
  brights_enter_user_from_syscall(
    (void *)(uintptr_t)p->user_entry,
    (void *)(uintptr_t)p->user_stack_top,
    p->cr3
  );

  return 0;
}
#endif

/* Fork: create a child process that is a copy of the parent */
int brights_proc_fork(void)
{
  brights_proc_info_t *parent = proc_get_current();
  if (!parent) return -1;

  int child_idx = proc_find_free_slot();
  if (child_idx < 0) return -1;

  brights_proc_info_t *child = &proc_table[child_idx];

  /* Copy parent's process info */
  child->pid = proc_alloc_pid();
  child->ppid = parent->pid;
  child->state = BRIGHTS_PROC_RUNNABLE;
  child->exit_code = 0;
  proc_str_copy(child->name, BRIGHTS_PROC_NAME_LEN, parent->name);
  proc_str_copy(child->cwd, BRIGHTS_PROC_CWD_LEN, parent->cwd);
  child->is_user = parent->is_user;

  /* Copy file descriptors (reference, not duplicate) */
  for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
    child->fd_table[j] = parent->fd_table[j];
  }

  /* Clone address space with COW */
  if (parent->is_user && parent->cr3 != 0) {
    child->cr3 = brights_paging_clone_address_space(parent->cr3);
    if (child->cr3 == 0) {
      child->state = BRIGHTS_PROC_UNUSED;
      return -1;
    }
  } else {
    child->cr3 = 0;
  }

  /* Copy user context */
  child->user_entry = parent->user_entry;
  child->user_stack_top = parent->user_stack_top;
  child->brk = parent->brk;
  child->brk_start = parent->brk_start;

  /* Allocate kernel stack for child */
  void *kstack_page = brights_pmem_alloc_page();
  if (kstack_page) {
    child->kernel_stack = (uint64_t)(uintptr_t)kstack_page + BRIGHTS_PROC_KSTACK_SIZE;
  } else {
    child->kernel_stack = 0;
  }

  /* Copy saved context from parent */
  child->ctx = parent->ctx;

  /* Reset scheduler stats for child */
  child->sched = parent->sched;
  child->sched.cpu_ticks = 0;
  child->sched.sched_count = 0;
  child->sched.last_sched_tick = 0;
  child->sched.total_wait_ticks = 0;

  /* Child returns 0 (rax = 0), parent returns child pid */
  child->ctx.rax = 0;

  pid_to_index[child->pid] = (uint32_t)child_idx;
  proc_mark_slot_used(child_idx);

  return (int)child->pid;
}
