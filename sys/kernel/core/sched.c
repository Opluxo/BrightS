#include <stdint.h>
#include "proc.h"
#include "sched.h"
#include "../arch/x86_64/tss.h"
#include "../arch/x86_64/paging.h"
#include "../arch/x86_64/trap.h"

#define BRIGHTS_SCHED_MAX_PROC 64u
#define BRIGHTS_SCHED_QUANTUM 10u

static uint64_t sched_ticks;
static uint64_t sched_dispatches;
static uint32_t pid_to_slot[64];
static uint32_t slot_count = 0;
static uint64_t runnable_bitmap = 0;
static uint32_t current_slot = 0;
static uint32_t current_pid = 0;
static uint32_t total_runnable = 0;
static brights_trap_frame_t *current_trap_frame = 0;
static uint32_t timeslice_remaining = BRIGHTS_SCHED_QUANTUM;

extern brights_proc_info_t *brights_proc_table_ptr(void);

static inline brights_proc_info_t *get_proc_by_slot(uint32_t slot)
{
  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table || slot >= slot_count) return 0;
  return &table[pid_to_slot[slot]];
}

static inline uint32_t find_next_runnable(uint32_t start)
{
  if (runnable_bitmap == 0) return 0xFFFFFFFF;

  uint64_t bitmap = runnable_bitmap;
  if (start > 0 && start < 64) {
    uint64_t mask = ~((1ULL << start) - 1);
    uint64_t high = bitmap & mask;
    if (high) return (uint32_t)__builtin_ctzll(high);
  }

  uint64_t low = bitmap & ((1ULL << start) - 1);
  if (low) return (uint32_t)__builtin_ctzll(low);

  return (uint32_t)__builtin_ctzll(bitmap);
}

void brights_sched_init(void)
{
  sched_ticks = 0;
  sched_dispatches = 0;
  runnable_bitmap = 0;
  current_slot = 0;
  current_pid = 0;
  total_runnable = 0;
  slot_count = 0;
  timeslice_remaining = BRIGHTS_SCHED_QUANTUM;
}

void brights_sched_tick(void)
{
  ++sched_ticks;
  if (current_pid != 0) {
    if (--timeslice_remaining == 0) {
      brights_sched_yield();
    }
  }
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
  timeslice_remaining = BRIGHTS_SCHED_QUANTUM;
  return 0;
}

int brights_sched_add_process(uint32_t pid)
{
  if (pid == 0 || slot_count >= BRIGHTS_SCHED_MAX_PROC) return -1;

  /* Check if already added */
  for (uint32_t i = 0; i < slot_count; ++i) {
    if (pid_to_slot[i] == pid) return 0;
  }

  /* Find proc table index for this pid */
  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) return -1;

  for (uint32_t i = 0; i < BRIGHTS_SCHED_MAX_PROC; ++i) {
    if (table[i].pid == pid) {
      uint32_t slot = slot_count++;
      pid_to_slot[slot] = i;
      /* Mark as runnable */
      runnable_bitmap |= (1ULL << slot);
      ++total_runnable;
      return 0;
    }
  }

  return -1;
}

int brights_sched_remove_process(uint32_t pid)
{
  for (uint32_t i = 0; i < slot_count; ++i) {
    brights_proc_info_t *p = get_proc_by_slot(i);
    if (p && p->pid == pid) {
      /* Clear from bitmap */
      if (runnable_bitmap & (1ULL << i)) {
        --total_runnable;
      }
      runnable_bitmap &= ~(1ULL << i);

      /* Compact the slot array */
      for (uint32_t j = i; j < slot_count - 1; ++j) {
        pid_to_slot[j] = pid_to_slot[j + 1];
      }
      --slot_count;

      if (current_slot >= slot_count && slot_count > 0)
        current_slot = 0;
      return 0;
    }
  }
  return -1;
}

int brights_sched_schedule(void)
{
  if (total_runnable == 0) {
    current_pid = 0;
    return -1;
  }

  /* Find next runnable with BSF */
  uint32_t next = find_next_runnable(current_slot);
  if (next == 0xFFFFFFFF || next >= slot_count) {
    current_pid = 0;
    return -1;
  }

  current_slot = next;
  brights_proc_info_t *p = get_proc_by_slot(current_slot);
  if (!p) {
    current_pid = 0;
    return -1;
  }

  current_pid = p->pid;
  p->state = BRIGHTS_PROC_RUNNING;
  brights_sched_mark_dispatch();
  return 0;
}

uint32_t brights_sched_current_pid(void)
{
  return current_pid;
}

void brights_sched_set_trap_frame(void *tf)
{
  current_trap_frame = (brights_trap_frame_t *)tf;
}

void *brights_sched_get_trap_frame(void)
{
  return current_trap_frame;
}

static inline void save_context(uint32_t slot)
{
  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (!p || !current_trap_frame) return;

  p->ctx.r15 = current_trap_frame->r15;
  p->ctx.r14 = current_trap_frame->r14;
  p->ctx.r13 = current_trap_frame->r13;
  p->ctx.r12 = current_trap_frame->r12;
  p->ctx.r11 = current_trap_frame->r11;
  p->ctx.r10 = current_trap_frame->r10;
  p->ctx.r9  = current_trap_frame->r9;
  p->ctx.r8  = current_trap_frame->r8;
  p->ctx.rbp = current_trap_frame->rbp;
  p->ctx.rdi = current_trap_frame->rdi;
  p->ctx.rsi = current_trap_frame->rsi;
  p->ctx.rdx = current_trap_frame->rdx;
  p->ctx.rcx = current_trap_frame->rcx;
  p->ctx.rbx = current_trap_frame->rbx;
  p->ctx.rax = current_trap_frame->rax;
  p->ctx.rip = current_trap_frame->rip;
  p->ctx.cs  = current_trap_frame->cs;
  p->ctx.rflags = current_trap_frame->rflags;
  p->ctx.rsp = current_trap_frame->rsp;
  p->ctx.ss  = current_trap_frame->ss;
}

static inline void restore_context(uint32_t slot)
{
  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (!p || !current_trap_frame) return;

  current_trap_frame->r15 = p->ctx.r15;
  current_trap_frame->r14 = p->ctx.r14;
  current_trap_frame->r13 = p->ctx.r13;
  current_trap_frame->r12 = p->ctx.r12;
  current_trap_frame->r11 = p->ctx.r11;
  current_trap_frame->r10 = p->ctx.r10;
  current_trap_frame->r9  = p->ctx.r9;
  current_trap_frame->r8  = p->ctx.r8;
  current_trap_frame->rbp = p->ctx.rbp;
  current_trap_frame->rdi = p->ctx.rdi;
  current_trap_frame->rsi = p->ctx.rsi;
  current_trap_frame->rdx = p->ctx.rdx;
  current_trap_frame->rcx = p->ctx.rcx;
  current_trap_frame->rbx = p->ctx.rbx;
  current_trap_frame->rax = p->ctx.rax;
  current_trap_frame->rip = p->ctx.rip;
  current_trap_frame->cs  = p->ctx.cs;
  current_trap_frame->rflags = p->ctx.rflags;
  current_trap_frame->rsp = p->ctx.rsp;
  current_trap_frame->ss  = p->ctx.ss;

  if (p->is_user && p->cr3 != 0) {
    brights_paging_set_cr3(p->cr3);
  }

  if (p->is_user && p->kernel_stack > 0) {
    brights_tss_t *tss = brights_tss_ptr();
    tss->rsp0 = p->kernel_stack;
  }
}

int brights_sched_yield(void)
{
  if (current_pid == 0 || total_runnable <= 1) return -1;

  /* Save current process context */
  save_context(current_slot);

  /* Mark current as runnable */
  brights_proc_info_t *cur = get_proc_by_slot(current_slot);
  if (cur && cur->state == BRIGHTS_PROC_RUNNING) {
    cur->state = BRIGHTS_PROC_RUNNABLE;
  }

  /* Find next runnable (skip current) */
  uint32_t next = find_next_runnable(current_slot + 1);
  if (next == 0xFFFFFFFF || next >= slot_count) return -1;

  current_slot = next;
  brights_proc_info_t *p = get_proc_by_slot(current_slot);
  if (!p) return -1;

  current_pid = p->pid;
  p->state = BRIGHTS_PROC_RUNNING;
  brights_sched_mark_dispatch();

  /* Restore next process context */
  restore_context(current_slot);

  return 0;
}
