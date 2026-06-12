#include <stdint.h>
#include "proc.h"
#include "sched.h"
#ifdef __i386__
#include "../arch/i386/gdt.h"
#include "../arch/i386/paging.h"
#include "../arch/i386/trap.h"
#else
#include "../arch/x86_64/tss.h"
#include "../arch/x86_64/paging.h"
#include "../arch/x86_64/trap.h"
#endif

#define BRIGHTS_SCHED_MAX_PROC 64u

/* Time slice per priority level (ticks) */
static const uint8_t prio_quantum[BRIGHTS_SCHED_PRIO_CNT] = {5, 10, 20};

/* Nice-to-priority mapping thresholds */
#define NICE_HIGH_THRESH (-10)
#define NICE_LOW_THRESH   10

/* Aging: boost a LOW process to NORMAL after this many ticks without being scheduled */
#define AGING_BOOST_TICKS    100u
/* Full priority boost interval (all processes) */
#define AGING_FULL_INTERVAL  500u

/* ===== Data structures ===== */

typedef struct {
  uint32_t pid;         /* process table index */
  uint8_t  priority;    /* current priority level */
  uint8_t  in_use;      /* 1 if slot allocated */
} sched_slot_t;

static sched_slot_t   slot_table[BRIGHTS_SCHED_MAX_PROC];
static uint64_t       free_slot_bitmap;             /* 1 = free */
static uint64_t       run_queues[BRIGHTS_SCHED_PRIO_CNT]; /* bitmap per priority */
static uint8_t        slot_of_pid[256];             /* pid → slot (0xFF = none) */

static uint64_t       sched_ticks;
static uint64_t       sched_dispatches;
static uint32_t       current_pid;
static int32_t        current_slot;
static uint32_t       total_runnable;
static uint32_t       timeslice_remaining;
static brights_trap_frame_t *current_trap_frame;

extern brights_proc_info_t *brights_proc_table_ptr(void);

/* ===== Helpers ===== */

static inline brights_proc_info_t *get_proc_by_slot(int32_t slot)
{
  if (slot < 0 || (uint32_t)slot >= BRIGHTS_SCHED_MAX_PROC) return 0;
  if (!slot_table[slot].in_use) return 0;
  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) return 0;
  return &table[slot_table[slot].pid];
}

static inline int32_t alloc_slot(void)
{
  uint64_t mask = (BRIGHTS_SCHED_MAX_PROC == 64) ? ~0ULL : ((1ULL << BRIGHTS_SCHED_MAX_PROC) - 1);
  uint64_t avail = ~free_slot_bitmap & mask;
  if (avail == 0) return -1;
  int32_t slot = (int32_t)__builtin_ctzll(avail);
  free_slot_bitmap &= ~(1ULL << slot);
  slot_table[slot].in_use = 0;
  return slot;
}

static inline void free_slot(int32_t slot)
{
  if (slot < 0 || (uint32_t)slot >= BRIGHTS_SCHED_MAX_PROC) return;
  for (int p = 0; p < BRIGHTS_SCHED_PRIO_CNT; ++p)
    run_queues[p] &= ~(1ULL << slot);
  if (slot_table[slot].in_use && slot_table[slot].pid < 256)
    slot_of_pid[slot_table[slot].pid] = 0xFF;
  slot_table[slot].in_use = 0;
  free_slot_bitmap |= (1ULL << slot);
  if (current_slot == slot) current_slot = -1;
}

static inline void set_runnable(int32_t slot, uint8_t prio)
{
  if (slot < 0) return;
  run_queues[prio] |= (1ULL << slot);
  total_runnable++;
}

static inline void clear_runnable(int32_t slot, uint8_t prio)
{
  if (slot < 0) return;
  if (run_queues[prio] & (1ULL << slot))
    total_runnable--;
  run_queues[prio] &= ~(1ULL << slot);
}

/* Find the next runnable process, preferring higher priority.
 * Returns slot index or -1 if none. */
static int32_t find_next_runnable(void)
{
  for (int p = 0; p < BRIGHTS_SCHED_PRIO_CNT; ++p) {
    uint64_t bm = run_queues[p];
    if (bm == 0) continue;

    /* If current is still runnable at this priority, advance past it */
    int32_t start = (current_slot >= 0 && (uint32_t)current_slot < BRIGHTS_SCHED_MAX_PROC) ? current_slot : -1;
    if (start >= 0) {
      uint64_t rest = bm & ~((1ULL << (start + 1)) - 1);
      if (rest) return (int32_t)__builtin_ctzll(rest);
    }

    /* Wrap around */
    if (bm) return (int32_t)__builtin_ctzll(bm);
  }
  return -1;
}

/* Map nice value to base priority level */
static inline uint8_t nice_to_priority(int32_t nice)
{
  if (nice <= NICE_HIGH_THRESH) return BRIGHTS_SCHED_PRIO_HIGH;
  if (nice >= NICE_LOW_THRESH)  return BRIGHTS_SCHED_PRIO_LOW;
  return BRIGHTS_SCHED_PRIO_NORMAL;
}

/* ===== Public API ===== */

void brights_sched_init(void)
{
  sched_ticks = 0;
  sched_dispatches = 0;
  current_pid = 0;
  current_slot = -1;
  total_runnable = 0;
  timeslice_remaining = 0;
  current_trap_frame = 0;
  free_slot_bitmap = (BRIGHTS_SCHED_MAX_PROC == 64) ? ~0ULL : ((1ULL << BRIGHTS_SCHED_MAX_PROC) - 1);

  for (uint32_t i = 0; i < BRIGHTS_SCHED_MAX_PROC; ++i) {
    slot_table[i].in_use = 0;
    slot_table[i].pid = 0;
    slot_table[i].priority = BRIGHTS_SCHED_PRIO_NORMAL;
  }
  for (int i = 0; i < BRIGHTS_SCHED_PRIO_CNT; ++i)
    run_queues[i] = 0;
  for (int i = 0; i < 256; ++i)
    slot_of_pid[i] = 0xFF;
}

void brights_sched_tick(void)
{
  ++sched_ticks;

  /* Periodic priority boost to prevent starvation */
  if ((sched_ticks % AGING_FULL_INTERVAL) == 0) {
    brights_proc_info_t *table = brights_proc_table_ptr();
    if (table) {
      for (uint32_t i = 0; i < BRIGHTS_SCHED_MAX_PROC; ++i) {
        if (!slot_table[i].in_use) continue;
        uint32_t pid_idx = slot_table[i].pid;
        if (table[pid_idx].state == BRIGHTS_PROC_UNUSED) continue;

        /* Boost priority for processes that haven't been scheduled */
        uint64_t ticks_since_sched = sched_ticks - table[pid_idx].sched.last_sched_tick;
        uint8_t cur_prio = slot_table[i].priority;

        if (cur_prio != BRIGHTS_SCHED_PRIO_HIGH &&
            ticks_since_sched > AGING_BOOST_TICKS) {
          uint8_t new_prio = cur_prio - 1; /* boost one level */
          clear_runnable(i, cur_prio);
          slot_table[i].priority = new_prio;
          if (table[pid_idx].state == BRIGHTS_PROC_RUNNABLE)
            set_runnable(i, new_prio);
          table[pid_idx].sched.boost_scheduled = sched_ticks;
        }
      }
    }
  }

  /* Preempt current process if timeslice expired */
  if (current_pid != 0 && --timeslice_remaining == 0) {
    brights_sched_yield();
  }
}

uint64_t brights_sched_ticks(void) { return sched_ticks; }
uint64_t brights_sched_dispatches(void) { return sched_dispatches; }

int brights_sched_mark_dispatch(void)
{
  ++sched_dispatches;
  if (current_slot >= 0) {
    uint8_t prio = slot_table[current_slot].priority;
    timeslice_remaining = prio_quantum[prio];
  } else {
    timeslice_remaining = prio_quantum[BRIGHTS_SCHED_PRIO_NORMAL];
  }
  return 0;
}

int brights_sched_add_process(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return -1;
  if (slot_of_pid[pid] != 0xFF) return 0;

  int32_t slot = alloc_slot();
  if (slot < 0) return -1;

  uint32_t pid_idx = brights_proc_index(pid);
  if (pid_idx >= BRIGHTS_SCHED_MAX_PROC) { free_slot(slot); return -1; }

  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) { free_slot(slot); return -1; }

  uint8_t prio = nice_to_priority(table[pid_idx].sched.nice);
  slot_table[slot].pid = pid_idx;
  slot_table[slot].priority = prio;
  slot_table[slot].in_use = 1;
  slot_of_pid[pid] = (uint8_t)slot;

  if (table[pid_idx].state == BRIGHTS_PROC_RUNNABLE) {
    set_runnable(slot, prio);
  }

  table[pid_idx].sched.cpu_ticks = 0;
  table[pid_idx].sched.sched_count = 0;
  table[pid_idx].sched.last_sched_tick = sched_ticks;
  table[pid_idx].sched.total_wait_ticks = 0;
  table[pid_idx].sched.priority = prio;
  table[pid_idx].sched.nice = 0;
  table[pid_idx].sched.boost_scheduled = sched_ticks;

  return 0;
}

int brights_sched_remove_process(uint32_t pid)
{
  if (pid == 0 || pid >= 256) return -1;
  uint8_t slot = slot_of_pid[pid];
  if (slot == 0xFF) return -1;

  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (p) p->state = BRIGHTS_PROC_UNUSED;

  free_slot(slot);
  return 0;
}

int brights_sched_schedule(void)
{
  if (total_runnable == 0) {
    current_pid = 0;
    current_slot = -1;
    return -1;
  }

  int32_t slot = find_next_runnable();
  if (slot < 0 || (uint32_t)slot >= BRIGHTS_SCHED_MAX_PROC) {
    current_pid = 0;
    current_slot = -1;
    return -1;
  }

  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (!p) {
    free_slot(slot);
    return -1;
  }

  current_slot = slot;
  current_pid = p->pid;
  p->state = BRIGHTS_PROC_RUNNING;

  p->sched.sched_count++;
  p->sched.last_sched_tick = sched_ticks;

  brights_sched_mark_dispatch();
  return 0;
}

uint32_t brights_sched_current_pid(void) { return current_pid; }

void brights_sched_set_trap_frame(void *tf)
{
  current_trap_frame = (brights_trap_frame_t *)tf;
}

void *brights_sched_get_trap_frame(void)
{
  return current_trap_frame;
}

/* ===== Context save / restore (unchanged from original) ===== */

static inline void save_context(int32_t slot)
{
#ifdef __i386__
  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (!p || !current_trap_frame) return;

  if (current_trap_frame->cs & 3) {
    p->ctx.rsp = current_trap_frame->user_esp;
    p->ctx.ss  = current_trap_frame->ss;
  } else {
    p->ctx.rsp = current_trap_frame->esp;
    p->ctx.ss  = BRIGHTS_KERNEL_DS;
  }
  p->ctx.rax = current_trap_frame->eax;
  p->ctx.rbx = current_trap_frame->ebx;
  p->ctx.rcx = current_trap_frame->ecx;
  p->ctx.rdx = current_trap_frame->edx;
  p->ctx.rsi = current_trap_frame->esi;
  p->ctx.rdi = current_trap_frame->edi;
  p->ctx.rbp = current_trap_frame->ebp;
  p->ctx.rip = current_trap_frame->eip;
  p->ctx.cs  = current_trap_frame->cs;
  p->ctx.rflags = current_trap_frame->eflags;
#else
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
#endif
}

static inline void restore_context(int32_t slot)
{
#ifdef __i386__
  brights_proc_info_t *p = get_proc_by_slot(slot);
  if (!p || !current_trap_frame) return;

  current_trap_frame->eax = p->ctx.rax;
  current_trap_frame->ecx = p->ctx.rcx;
  current_trap_frame->edx = p->ctx.rdx;
  current_trap_frame->ebx = p->ctx.rbx;
  current_trap_frame->esi = p->ctx.rsi;
  current_trap_frame->edi = p->ctx.rdi;
  current_trap_frame->ebp = p->ctx.rbp;
  current_trap_frame->eip = p->ctx.rip;
  current_trap_frame->cs  = p->ctx.cs;
  current_trap_frame->eflags = p->ctx.rflags;

  if (p->ctx.cs & 3) {
    current_trap_frame->user_esp = p->ctx.rsp;
    current_trap_frame->ss = p->ctx.ss;
  } else {
    current_trap_frame->esp = p->ctx.rsp;
    current_trap_frame->ss = BRIGHTS_KERNEL_DS;
  }

  if (p->is_user && p->cr3 != 0) {
    brights_paging_set_cr3(p->cr3);
  }

  if (p->is_user && p->kernel_stack > 0) {
    extern void brights_tss_init(uint32_t esp0);
    brights_tss_init(p->kernel_stack);
  }
#else
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
#endif
}

int brights_sched_yield(void)
{
  if (current_pid == 0) return -1;

  /* Account CPU time to the current process */
  if (current_slot >= 0) {
    brights_proc_info_t *cur = get_proc_by_slot(current_slot);
    if (cur) {
      uint64_t used = prio_quantum[slot_table[current_slot].priority] - timeslice_remaining;
      cur->sched.cpu_ticks += used;

      /* Save context before switching */
      save_context(current_slot);

      uint8_t old_prio = slot_table[current_slot].priority;
      clear_runnable(current_slot, old_prio);

      /* Mark current as runnable */
      if (cur->state == BRIGHTS_PROC_RUNNING) {
        cur->state = BRIGHTS_PROC_RUNNABLE;
      }

      /* ZOMBIE or UNUSED processes are gone — don't re-queue */
      if (cur->state == BRIGHTS_PROC_RUNNABLE) {
        /* Dynamic priority adjustment:
         * - Used full timeslice → demote (CPU-bound)
         * - Yielded early (timeslice left) → promote or stay (I/O-bound) */
        if (timeslice_remaining == 0 && old_prio < BRIGHTS_SCHED_PRIO_LOW) {
          uint8_t new_prio = old_prio + 1;
          slot_table[current_slot].priority = new_prio;
          cur->sched.priority = new_prio;
          set_runnable(current_slot, new_prio);
        } else if (timeslice_remaining > 0 && old_prio > BRIGHTS_SCHED_PRIO_HIGH) {
          uint8_t new_prio = old_prio - 1;
          slot_table[current_slot].priority = new_prio;
          cur->sched.priority = new_prio;
          set_runnable(current_slot, new_prio);
        } else {
          set_runnable(current_slot, old_prio);
        }
      }
    }
  }

  if (total_runnable == 0) {
    current_pid = 0;
    current_slot = -1;
    return -1;
  }

  /* Find next runnable process */
  int32_t next = find_next_runnable();
  if (next < 0) {
    current_pid = 0;
    current_slot = -1;
    return -1;
  }

  current_slot = next;
  brights_proc_info_t *p = get_proc_by_slot(current_slot);
  if (!p) {
    free_slot(current_slot);
    current_pid = 0;
    current_slot = -1;
    return -1;
  }

  current_pid = p->pid;
  p->state = BRIGHTS_PROC_RUNNING;
  p->sched.sched_count++;
  p->sched.last_sched_tick = sched_ticks;
  brights_sched_mark_dispatch();

  restore_context(current_slot);
  return 0;
}

/* ===== Priority / resource control ===== */

int brights_sched_set_nice(uint32_t pid, int32_t nice)
{
  if (pid == 0 || pid >= 256) return -1;
  if (nice < -20) nice = -20;
  if (nice > 19)  nice = 19;

  uint32_t pid_idx = brights_proc_index(pid);
  if (pid_idx >= BRIGHTS_SCHED_MAX_PROC) return -1;

  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) return -1;

  table[pid_idx].sched.nice = nice;

  /* Update scheduler slot priority if process is tracked */
  uint8_t slot = slot_of_pid[pid];
  if (slot != 0xFF) {
    uint8_t new_prio = nice_to_priority(nice);
    uint8_t old_prio = slot_table[slot].priority;
    if (new_prio != old_prio) {
      clear_runnable(slot, old_prio);
      slot_table[slot].priority = new_prio;
      if (table[pid_idx].state == BRIGHTS_PROC_RUNNABLE)
        set_runnable(slot, new_prio);
    }
  }

  return 0;
}

int brights_sched_get_stats(uint32_t pid, brights_proc_sched_t *out)
{
  if (!out || pid == 0 || pid >= 256) return -1;

  uint32_t pid_idx = brights_proc_index(pid);
  if (pid_idx >= BRIGHTS_SCHED_MAX_PROC) return -1;

  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table || table[pid_idx].state == BRIGHTS_PROC_UNUSED) return -1;

  *out = table[pid_idx].sched;
  return 0;
}
