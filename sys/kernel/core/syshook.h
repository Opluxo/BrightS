#ifndef BRIGHTS_SYSHOOK_H
#define BRIGHTS_SYSHOOK_H

#include <stdint.h>

/* Maximum concurrent hooks system-wide */
#define SYSHOOK_MAX 32

/* Ring buffer size per hook (must be power of 2) */
#define SYSHOOK_RINGBUF_SIZE 64

/* Watch mask covers syscalls 0-127 (2 x 64-bit words) */
#define SYSHOOK_WATCH_ALL_LO  ~0ULL
#define SYSHOOK_WATCH_ALL_HI  ~0ULL
#define SYSHOOK_WATCH_NONE_LO 0ULL
#define SYSHOOK_WATCH_NONE_HI 0ULL

/* Hook flags */
#define HOOK_FLAG_PRE       (1u << 0)  /* Notify before syscall executes */
#define HOOK_FLAG_POST      (1u << 1)  /* Notify after syscall returns */
#define HOOK_FLAG_BROADCAST (1u << 2)  /* Receive user broadcasts */
#define HOOK_FLAG_DEFAULT   (HOOK_FLAG_PRE | HOOK_FLAG_POST | HOOK_FLAG_BROADCAST)

/* Event types embedded in brights_hook_event_t.event_type */
#define HOOK_EVT_PRE_SYSCALL  0  /* Before syscall execution */
#define HOOK_EVT_POST_SYSCALL 1  /* After syscall execution (ret valid) */
#define HOOK_EVT_BROADCAST    2  /* User-defined broadcast */

/* Special syscall number in watch_mask: watch all */
#define SYSHOOK_SYSCALL_ALL  0xFF

/*
 * Hook event — delivered from kernel to user via ring buffer.
 * Exactly 64 bytes: one cache line, no false sharing.
 */
typedef struct {
  uint64_t event_type;    /* HOOK_EVT_* */
  uint64_t syscall_nr;    /* Which syscall (0 for broadcast) */
  uint64_t args[6];       /* Syscall arguments */
  int64_t  ret;           /* Return value (valid for POST) */
  uint32_t caller_pid;    /* PID of process that triggered */
  uint32_t pad0;
  uint64_t timestamp;     /* TSC at event time */
  uint64_t custom[2];     /* Custom broadcast payload */
} __attribute__((aligned(64))) brights_hook_event_t;

/* Hook registration info — returned to user via hook_info syscall */
typedef struct {
  uint32_t hook_id;
  uint32_t owner_pid;
  uint32_t flags;
  uint32_t event_count;    /* Events in ringbuf right now */
  uint64_t total_events;   /* Total events dispatched */
  uint64_t dropped_events; /* Events dropped (ringbuf full) */
} brights_hook_info_t;

/*
 * Internal hook entry — kernel-only, not exposed to user space.
 */
typedef struct {
  int active;
  uint32_t owner_pid;
  uint64_t watch_mask[2]; /* Bitmask for syscall 0-127 */
  uint32_t flags;

  /* Per-hook ring buffer (kernel memory) */
  brights_hook_event_t ringbuf[SYSHOOK_RINGBUF_SIZE];
  volatile uint32_t rd;   /* Read index */
  volatile uint32_t wr;   /* Write index */
  volatile uint32_t count;/* Number of pending events */

  /* Stats */
  uint64_t total_events;
  uint64_t dropped_events;
} brights_hook_entry_t;

/* ===== Kernel-side API ===== */

/* Initialize hook subsystem (called once at boot) */
void brights_syshook_init(void);

/*
 * Notify all matching hooks of a syscall event.
 * Called from brights_syscall_dispatch().
 *   phase: HOOK_EVT_PRE_SYSCALL or HOOK_EVT_POST_SYSCALL
 *   num:   syscall number
 *   args:  6 syscall arguments
 *   ret:   return value (valid only for POST)
 *   pid:   caller process PID
 *   tsc:   TSC timestamp
 */
void brights_syshook_notify(uint64_t phase,
                             uint64_t num,
                             const uint64_t *args,
                             int64_t ret,
                             uint32_t pid,
                             uint64_t tsc);

/*
 * Broadcast a custom event to all HOOK_FLAG_BROADCAST hooks.
 *   sender_pid: PID of broadcasting process
 *   custom_a:   first custom payload word
 *   custom_b:   second custom payload word
 */
void brights_syshook_broadcast(uint32_t sender_pid,
                                uint64_t custom_a,
                                uint64_t custom_b);

/* ===== Syscall implementations (called from sysent.c) ===== */

/* sys_hook_register(watch_lo, watch_hi, flags) → hook_id or -1 */
int64_t brights_sys_hook_register(uint64_t watch_lo, uint64_t watch_hi, uint64_t flags);

/* sys_hook_unregister(hook_id) → 0 or -1 */
int64_t brights_sys_hook_unregister(uint64_t hook_id);

/* sys_hook_poll(hook_id, event_buf_va, max_events) → count or -1 */
int64_t brights_sys_hook_poll(uint64_t hook_id, uint64_t event_buf_va, uint64_t max_events);

/* sys_hook_wait(hook_id, event_buf_va, timeout_ms) → count or -1 */
int64_t brights_sys_hook_wait(uint64_t hook_id, uint64_t event_buf_va, uint64_t timeout_ms);

/* sys_hook_info(hook_id, info_buf_va) → 0 or -1 */
int64_t brights_sys_hook_info(uint64_t hook_id, uint64_t info_buf_va);

/* sys_broadcast(custom_a, custom_b) → 0 */
int64_t brights_sys_broadcast(uint64_t custom_a, uint64_t custom_b);

/* ===== Process cleanup ===== */

/* Release all hooks owned by the given PID (called on proc exit) */
void brights_syshook_cleanup_pid(uint32_t pid);

/* ===== Statistics ===== */

/* Number of currently active hooks */
uint64_t brights_syshook_active_count(void);

/* Total hooks created since boot */
uint64_t brights_syshook_total_created(void);

/* Total events dispatched since boot */
uint64_t brights_syshook_total_events(void);

/* ===== Debug access ===== */

/* Get raw hook entry pointer for lightshell display. Returns 0 on success. */
int brights_syshook_get_entry(int slot, brights_hook_entry_t **out);

#endif
