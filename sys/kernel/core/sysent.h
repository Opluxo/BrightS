#ifndef BRIGHTS_SYSENT_H
#define BRIGHTS_SYSENT_H

#include <stdint.h>

typedef int64_t (*brights_syscall_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

typedef struct {
  uint8_t argc;
  brights_syscall_fn fn;
} brights_sysent_t;

extern brights_sysent_t brights_sysent_table[];
extern const uint64_t brights_sysent_count;

#endif
