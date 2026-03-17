#include "syscall.h"
#include "sysent.h"

int64_t brights_syscall_dispatch(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  if (num >= brights_sysent_count) {
    return -1;
  }
  brights_sysent_t *ent = &brights_sysent_table[num];
  if (!ent->fn) {
    return -1;
  }
  return ent->fn(a0, a1, a2, a3, a4, a5);
}
