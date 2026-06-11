#ifndef BRIGHTS_SYSCALL_H
#define BRIGHTS_SYSCALL_H

#include <stdint.h>

int64_t brights_syscall_dispatch(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);

#endif
