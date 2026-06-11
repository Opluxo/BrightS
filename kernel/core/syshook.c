#include "syshook.h"
#include "proc.h"
#include "sleep.h"
#include "clock.h"
#include <stdint.h>

/* ===== Global hook table ===== */
static brights_hook_entry_t hook_table[SYSHOOK_MAX];
static int syshook_initialized = 0;

/* Global statistics */
static uint64_t syshook_total_hooks_created = 0;
static uint64_t syshook_total_events_dispatched = 0;

/* Fast inline memcpy for event copy (avoids pulling in runtime.c dependency) */
static void hook_memcpy(void *dst, const void *src, uint64_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint64_t i = 0; i < n; ++i) d[i] = s[i];
}

/* ===== Initialization ===== */
void brights_syshook_init(void)
{
  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    hook_table[i].active = 0;
    hook_table[i].owner_pid = 0;
    hook_table[i].watch_mask[0] = 0;
    hook_table[i].watch_mask[1] = 0;
    hook_table[i].flags = 0;
    hook_table[i].rd = 0;
    hook_table[i].wr = 0;
    hook_table[i].count = 0;
    hook_table[i].total_events = 0;
    hook_table[i].dropped_events = 0;
  }
  syshook_initialized = 1;
}

/* ===== Ring buffer helpers ===== */

/* Push an event into a hook's ring buffer. Returns 0 on success, -1 if full. */
static int ringbuf_push(brights_hook_entry_t *hook, const brights_hook_event_t *evt)
{
  if (hook->count >= SYSHOOK_RINGBUF_SIZE) {
    /* Buffer full — drop oldest to make room (overwrite policy) */
    hook->rd = (hook->rd + 1) % SYSHOOK_RINGBUF_SIZE;
    hook->dropped_events++;
    /* Don't decrement count — we're about to overwrite */
  } else {
    hook->count++;
  }

  hook_memcpy(&hook->ringbuf[hook->wr], evt, sizeof(brights_hook_event_t));
  hook->wr = (hook->wr + 1) % SYSHOOK_RINGBUF_SIZE;
  hook->total_events++;
  return 0;
}

/* Pop up to max_events from ring buffer into dst. Returns count popped. */
static int ringbuf_pop(brights_hook_entry_t *hook, brights_hook_event_t *dst, uint32_t max_events)
{
  uint32_t n = 0;
  while (n < max_events && hook->count > 0) {
    hook_memcpy(&dst[n], &hook->ringbuf[hook->rd], sizeof(brights_hook_event_t));
    hook->rd = (hook->rd + 1) % SYSHOOK_RINGBUF_SIZE;
    hook->count--;
    n++;
  }
  return (int)n;
}

/* ===== Core: check if syscall nr matches watch mask ===== */
static inline int hook_watches_syscall(const brights_hook_entry_t *hook, uint64_t nr, uint64_t phase_flag)
{
  if (!(hook->flags & phase_flag)) return 0;
  if (nr < 64) {
    return (hook->watch_mask[0] >> nr) & 1;
  } else if (nr < 128) {
    return (hook->watch_mask[1] >> (nr - 64)) & 1;
  }
  return 0;
}

/* ===== Notify: called from brights_syscall_dispatch ===== */
void brights_syshook_notify(uint64_t phase,
                             uint64_t num,
                             const uint64_t *args,
                             int64_t ret,
                             uint32_t pid,
                             uint64_t tsc)
{
  if (!syshook_initialized) return;

  uint64_t phase_flag = (phase == HOOK_EVT_PRE_SYSCALL) ? HOOK_FLAG_PRE : HOOK_FLAG_POST;

  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    brights_hook_entry_t *h = &hook_table[i];
    if (!h->active) continue;

    if (hook_watches_syscall(h, num, phase_flag)) {
      brights_hook_event_t evt;
      evt.event_type = phase;
      evt.syscall_nr = num;
      evt.args[0] = args[0];
      evt.args[1] = args[1];
      evt.args[2] = args[2];
      evt.args[3] = args[3];
      evt.args[4] = args[4];
      evt.args[5] = args[5];
      evt.ret = ret;
      evt.caller_pid = pid;
      evt.pad0 = 0;
      evt.timestamp = tsc;
      evt.custom[0] = 0;
      evt.custom[1] = 0;

      ringbuf_push(h, &evt);
      syshook_total_events_dispatched++;
    }
  }
}

/* ===== Broadcast: custom event to all BROADCAST-flagged hooks ===== */
void brights_syshook_broadcast(uint32_t sender_pid,
                                uint64_t custom_a,
                                uint64_t custom_b)
{
  if (!syshook_initialized) return;

  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    brights_hook_entry_t *h = &hook_table[i];
    if (!h->active) continue;
    if (!(h->flags & HOOK_FLAG_BROADCAST)) continue;

    brights_hook_event_t evt;
    evt.event_type = HOOK_EVT_BROADCAST;
    evt.syscall_nr = 0;
    for (int j = 0; j < 6; ++j) evt.args[j] = 0;
    evt.ret = 0;
    evt.caller_pid = sender_pid;
    evt.pad0 = 0;
    evt.timestamp = 0;
    evt.custom[0] = custom_a;
    evt.custom[1] = custom_b;

    ringbuf_push(h, &evt);
  }
}

/* ===== Syscall: hook_register ===== */
int64_t brights_sys_hook_register(uint64_t watch_lo, uint64_t watch_hi, uint64_t flags)
{
  if (!syshook_initialized) return -1;

  uint32_t pid = brights_proc_current();

  /* Find a free slot */
  int slot = -1;
  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    if (!hook_table[i].active) {
      slot = i;
      break;
    }
  }
  if (slot < 0) return -1; /* No free slots */

  brights_hook_entry_t *h = &hook_table[slot];
  h->active = 1;
  h->owner_pid = pid;
  h->watch_mask[0] = watch_lo;
  h->watch_mask[1] = watch_hi;
  h->flags = (uint32_t)(flags & (HOOK_FLAG_PRE | HOOK_FLAG_POST | HOOK_FLAG_BROADCAST));
  h->rd = 0;
  h->wr = 0;
  h->count = 0;
  h->total_events = 0;
  h->dropped_events = 0;

  syshook_total_hooks_created++;
  return (int64_t)slot;
}

/* ===== Syscall: hook_unregister ===== */
int64_t brights_sys_hook_unregister(uint64_t hook_id)
{
  if (!syshook_initialized || hook_id >= SYSHOOK_MAX) return -1;

  brights_hook_entry_t *h = &hook_table[hook_id];
  if (!h->active) return -1;

  /* Only the owner can unregister (or root) */
  uint32_t pid = brights_proc_current();
  if (h->owner_pid != pid) return -1;

  h->active = 0;
  h->owner_pid = 0;
  h->watch_mask[0] = 0;
  h->watch_mask[1] = 0;
  h->flags = 0;
  h->rd = 0;
  h->wr = 0;
  h->count = 0;

  return 0;
}

/* ===== Syscall: hook_poll ===== */
int64_t brights_sys_hook_poll(uint64_t hook_id, uint64_t event_buf_va, uint64_t max_events)
{
  if (!syshook_initialized || hook_id >= SYSHOOK_MAX) return -1;
  if (event_buf_va == 0 || max_events == 0) return -1;

  brights_hook_entry_t *h = &hook_table[hook_id];
  if (!h->active) return -1;

  uint32_t pid = brights_proc_current();
  if (h->owner_pid != pid) return -1;

  /* Cap to ringbuf size */
  if (max_events > SYSHOOK_RINGBUF_SIZE) max_events = SYSHOOK_RINGBUF_SIZE;

  /* Allocate temp buffer in kernel, copy to user */
  brights_hook_event_t tmp[SYSHOOK_RINGBUF_SIZE];
  int count = ringbuf_pop(h, tmp, (uint32_t)max_events);
  if (count <= 0) return 0;

  /* Copy events to user-space buffer */
  uint64_t copy_size = (uint64_t)count * sizeof(brights_hook_event_t);
  hook_memcpy((void *)(uintptr_t)event_buf_va, tmp, copy_size);

  return (int64_t)count;
}

/* ===== Syscall: hook_wait ===== */
int64_t brights_sys_hook_wait(uint64_t hook_id, uint64_t event_buf_va, uint64_t timeout_ms)
{
  if (!syshook_initialized || hook_id >= SYSHOOK_MAX) return -1;
  if (event_buf_va == 0) return -1;

  brights_hook_entry_t *h = &hook_table[hook_id];
  if (!h->active) return -1;

  uint32_t pid = brights_proc_current();
  if (h->owner_pid != pid) return -1;

  /* Poll with timeout */
  uint64_t elapsed = 0;
  uint64_t tick_ms = 10; /* ~10ms per sched tick at 100Hz */

  while (h->count == 0) {
    if (timeout_ms > 0 && elapsed >= timeout_ms) return 0; /* Timeout */
    brights_sleep_ms(tick_ms);
    elapsed += tick_ms;
  }

  /* Now poll one event */
  brights_hook_event_t evt;
  int count = ringbuf_pop(h, &evt, 1);
  if (count <= 0) return 0;

  hook_memcpy((void *)(uintptr_t)event_buf_va, &evt, sizeof(brights_hook_event_t));
  return 1;
}

/* ===== Syscall: hook_info ===== */
int64_t brights_sys_hook_info(uint64_t hook_id, uint64_t info_buf_va)
{
  if (!syshook_initialized || hook_id >= SYSHOOK_MAX) return -1;
  if (info_buf_va == 0) return -1;

  brights_hook_entry_t *h = &hook_table[hook_id];
  if (!h->active) return -1;

  brights_hook_info_t info;
  info.hook_id = (uint32_t)hook_id;
  info.owner_pid = h->owner_pid;
  info.flags = h->flags;
  info.event_count = h->count;
  info.total_events = h->total_events;
  info.dropped_events = h->dropped_events;

  hook_memcpy((void *)(uintptr_t)info_buf_va, &info, sizeof(brights_hook_info_t));
  return 0;
}

/* ===== Syscall: broadcast ===== */
int64_t brights_sys_broadcast(uint64_t custom_a, uint64_t custom_b)
{
  uint32_t pid = brights_proc_current();
  brights_syshook_broadcast(pid, custom_a, custom_b);
  return 0;
}

/* ===== Cleanup: release all hooks owned by a PID ===== */
void brights_syshook_cleanup_pid(uint32_t pid)
{
  if (!syshook_initialized || pid == 0) return;

  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    brights_hook_entry_t *h = &hook_table[i];
    if (h->active && h->owner_pid == pid) {
      h->active = 0;
      h->owner_pid = 0;
      h->watch_mask[0] = 0;
      h->watch_mask[1] = 0;
      h->flags = 0;
      h->rd = 0;
      h->wr = 0;
      h->count = 0;
    }
  }
}

/* ===== Global statistics ===== */
uint64_t brights_syshook_active_count(void)
{
  if (!syshook_initialized) return 0;
  uint64_t count = 0;
  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    if (hook_table[i].active) count++;
  }
  return count;
}

uint64_t brights_syshook_total_created(void)
{
  return syshook_total_hooks_created;
}

uint64_t brights_syshook_total_events(void)
{
  return syshook_total_events_dispatched;
}

/* ===== Debug: get raw hook entry for lightshell ===== */
int brights_syshook_get_entry(int slot, brights_hook_entry_t **out)
{
  if (!syshook_initialized || slot < 0 || slot >= SYSHOOK_MAX || !out) return -1;
  *out = &hook_table[slot];
  return 0;
}
