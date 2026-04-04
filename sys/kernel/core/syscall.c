#include "syscall.h"
#include "sysent.h"
#include "syshook.h"
#include "proc.h"
#include "clock.h"

/* Read TSC inline */
static inline uint64_t rdtsc_fast(void)
{
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

int64_t brights_syscall_dispatch(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  if (num >= brights_sysent_count) {
    return -1;
  }
  brights_sysent_t *ent = &brights_sysent_table[num];
  if (!ent->handler) {
    return -1;
  }

  /* Gather context for hook notifications */
  uint64_t args[6] = { a0, a1, a2, a3, a4, a5 };
  uint32_t pid = brights_proc_current();
  uint64_t tsc = rdtsc_fast();

  /* Pre-hook: notify watchers before execution */
  brights_syshook_notify(HOOK_EVT_PRE_SYSCALL, num, args, 0, pid, tsc);

  /* Execute the actual syscall */
  int64_t ret = ent->handler(a0, a1, a2, a3, a4, a5);

  /* Post-hook: notify watchers after execution */
  tsc = rdtsc_fast();
  brights_syshook_notify(HOOK_EVT_POST_SYSCALL, num, args, ret, pid, tsc);

  return ret;
}
