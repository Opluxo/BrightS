#include "proc.h"
#include "pmem.h"
#include "syshook.h"
#include "kernel_util.h"
#include "elf.h"
#include "../arch/x86_64/paging.h"
#include "../drivers/serial.h"

#define BRIGHTS_PROC_MAX 64u

static brights_proc_info_t proc_table[BRIGHTS_PROC_MAX];
static uint32_t pid_to_index[256];  /* PID → table index (O(1) lookup) */
static uint64_t free_slots_bitmap;  /* Bit 0=free, 1=used */
static uint32_t next_pid = 1;
static uint32_t current_pid = 0;

/* Allow scheduler to access the proc table directly */
brights_proc_info_t *brights_proc_table_ptr(void)
{
  return proc_table;
}

/* Forward declarations */
static void proc_str_copy(char *dst, int cap, const char *src);
static brights_proc_info_t *proc_get_by_pid(uint32_t pid);
static brights_proc_info_t *proc_get_current(void);
static int proc_find_free_slot(void);
static void proc_mark_slot_used(int slot);


void brights_proc_init(void)
{
  free_slots_bitmap = 0;
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
  }
  next_pid = 1;
  current_pid = 0;
}

int brights_proc_spawn_kernel(const char *name)
{
  int slot = proc_find_free_slot();
  if (slot < 0) return -1;

  proc_table[slot].pid = next_pid++;
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

/* String helpers */
static void proc_str_copy(char *dst, int cap, const char *src)
{
  if (!dst || cap <= 0) return;
  int i = 0;
  while (src && src[i] && i < cap - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
}
/* O(1) PID lookup using direct mapping */
static brights_proc_info_t *proc_get_by_pid(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return 0;
  uint32_t idx = pid_to_index[pid];
  if (idx >= BRIGHTS_PROC_MAX) return 0;
  if (proc_table[idx].pid != pid || proc_table[idx].state == BRIGHTS_PROC_UNUSED) return 0;
  return &proc_table[idx];
}

/* Find current process slot - O(1) */
static brights_proc_info_t *proc_get_current(void)
{
  return proc_get_by_pid(current_pid);
}

/* Find free slot using bitmap - O(log n) */
static int proc_find_free_slot(void)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (!(free_slots_bitmap & (1ULL << i))) return (int)i;
  }
  return -1;
}

static void proc_mark_slot_used(int slot)
{
  free_slots_bitmap |= (1ULL << slot);
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
  if (!info_out || pid == 0) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      *info_out = proc_table[i];
      return 0;
    }
  }
  return -1;
}

int brights_proc_exit(uint32_t pid, int exit_code)
{
  if (pid == 0) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      /* Close all open file descriptors */
      for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
        if (proc_table[i].fd_table[j]) {
          brights_vfs2_close(proc_table[i].fd_table[j]);
          proc_table[i].fd_table[j] = 0;
        }
      }

      /* Clean up any syscall hooks owned by this process */
      brights_syshook_cleanup_pid(pid);

      /* Free the process address space (if not shared with kernel) */
      if (proc_table[i].cr3 != 0 && proc_table[i].is_user) {
        /* Only destroy if it's a user process with its own page table */
        uint64_t kernel_cr3 = brights_paging_get_cr3();
        if (proc_table[i].cr3 != kernel_cr3) {
          brights_paging_destroy_address_space(proc_table[i].cr3);
        }
      }

      /* Free kernel stack */
      if (proc_table[i].kernel_stack != 0) {
        void *kstack_page = (void *)(uintptr_t)(proc_table[i].kernel_stack - BRIGHTS_PROC_KSTACK_SIZE);
        brights_pmem_free_page(kstack_page);
      }

      proc_table[i].state = BRIGHTS_PROC_ZOMBIE;
      proc_table[i].exit_code = exit_code;
      return 0;
    }
  }
  return -1;
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

  /* Absolute path */
  if (path[0] == '/') {
    /* Normalize and copy */
    char segs[16][64];
    int seg_count = 0;
    int i = 0;
    while (path[i]) {
      while (path[i] == '/') ++i;
      if (!path[i]) break;
      char seg[64];
      int slen = 0;
      while (path[i] && path[i] != '/') {
        if (slen >= 63) return -1;
        seg[slen++] = path[i++];
      }
      seg[slen] = 0;
      if (slen == 1 && seg[0] == '.') continue;
      if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
        if (seg_count > 0) --seg_count;
        continue;
      }
      if (seg_count >= 16) return -1;
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

  /* Relative path: prepend cwd */
  /* Build: cwd + "/" + path, then normalize */
  char combined[256];
  int ci = 0;

  /* Copy cwd */
  for (int i = 0; cwd[i] && ci < 254; ++i) combined[ci++] = cwd[i];

  /* Add '/' if cwd doesn't end with one and path isn't empty */
  if (ci > 0 && combined[ci - 1] != '/' && path[0] != 0) {
    if (ci < 254) combined[ci++] = '/';
  }

  /* Copy relative path */
  for (int i = 0; path[i] && ci < 254; ++i) combined[ci++] = path[i];
  combined[ci] = 0;

  /* Normalize: resolve . and .. */
  char segs[16][64];
  int seg_count = 0;
  int i = 0;
  while (combined[i]) {
    while (combined[i] == '/') ++i;
    if (!combined[i]) break;
    char seg[64];
    int slen = 0;
    while (combined[i] && combined[i] != '/') {
      if (slen >= 63) return -1;
      seg[slen++] = combined[i++];
    }
    seg[slen] = 0;
    if (slen == 1 && seg[0] == '.') continue;
    if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
      if (seg_count > 0) --seg_count;
      continue;
    }
    if (seg_count >= 16) return -1;
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
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == BRIGHTS_PROC_UNUSED) {
      proc_table[i].pid = next_pid++;
      proc_table[i].ppid = current_pid;
      proc_table[i].state = BRIGHTS_PROC_RUNNABLE;
      proc_table[i].exit_code = 0;
      proc_table[i].is_user = 1;
      proc_table[i].user_entry = entry;
      proc_table[i].user_stack_top = stack_top;
      proc_table[i].brk = 0;
      proc_table[i].brk_start = 0;
      proc_table[i].cr3 = alloc_user_pml4();

      /* Allocate kernel stack for this process */
      void *kstack_page = brights_pmem_alloc_page();
      if (kstack_page) {
        proc_table[i].kernel_stack = (uint64_t)(uintptr_t)kstack_page + BRIGHTS_PROC_KSTACK_SIZE;
      } else {
        proc_table[i].kernel_stack = 0;
      }

      proc_str_copy(proc_table[i].name, BRIGHTS_PROC_NAME_LEN, name ? name : "user");

      /* Initialize fd table */
      for (int j = 0; j < BRIGHTS_PROC_MAX_FDS; ++j) {
        proc_table[i].fd_table[j] = 0;
      }

      /* Inherit cwd from parent */
      brights_proc_info_t *parent = proc_get_current();
      if (parent) {
        proc_str_copy(proc_table[i].cwd, BRIGHTS_PROC_CWD_LEN, parent->cwd);
      } else {
        proc_table[i].cwd[0] = '/';
        proc_table[i].cwd[1] = 0;
      }

      /* Initialize saved context */
      proc_table[i].ctx.rip = entry;
      proc_table[i].ctx.rsp = stack_top;
      proc_table[i].ctx.cs = 0x2B;   /* User CS */
      proc_table[i].ctx.ss = 0x23;   /* User DS */
      proc_table[i].ctx.rflags = 0x202; /* IF set */
      proc_table[i].ctx.rax = 0;
      proc_table[i].ctx.rbx = 0;
      proc_table[i].ctx.rcx = 0;
      proc_table[i].ctx.rdx = 0;
      proc_table[i].ctx.rsi = 0;
      proc_table[i].ctx.rdi = 0;
      proc_table[i].ctx.rbp = 0;
      proc_table[i].ctx.r8 = 0;
      proc_table[i].ctx.r9 = 0;
      proc_table[i].ctx.r10 = 0;
      proc_table[i].ctx.r11 = 0;
      proc_table[i].ctx.r12 = 0;
      proc_table[i].ctx.r13 = 0;
      proc_table[i].ctx.r14 = 0;
      proc_table[i].ctx.r15 = 0;

      return (int)proc_table[i].pid;
    }
  }
  return -1;
}

uint64_t brights_proc_get_cr3(uint32_t pid)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      return proc_table[i].cr3;
    }
  }
  return 0;
}

int brights_proc_set_brk(uint32_t pid, uint64_t new_brk)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      if (proc_table[i].brk_start == 0) {
        proc_table[i].brk_start = new_brk;
      }
      proc_table[i].brk = new_brk;
      return 0;
    }
  }
  return -1;
}

uint64_t brights_proc_get_brk(uint32_t pid)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      return proc_table[i].brk;
    }
  }
  return 0;
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
  p->ctx.cs = 0x2B;
  p->ctx.ss = 0x23;
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

/* Fork: create a child process that is a copy of the parent */
int brights_proc_fork(void)
{
  brights_proc_info_t *parent = proc_get_current();
  if (!parent) return -1;

  /* Find a free slot for the child */
  int child_idx = -1;
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == BRIGHTS_PROC_UNUSED) {
      child_idx = (int)i;
      break;
    }
  }
  if (child_idx < 0) return -1;

  brights_proc_info_t *child = &proc_table[child_idx];

  /* Copy parent's process info */
  child->pid = next_pid++;
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

  /* Child returns 0 (rax = 0), parent returns child pid */
  child->ctx.rax = 0;

  return (int)child->pid;
}
