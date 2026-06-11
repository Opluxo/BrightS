#ifndef BRIGHTS_X86_64_SYSCALL_ABI_H
#define BRIGHTS_X86_64_SYSCALL_ABI_H

#include <stdint.h>

#include "trap.h"

void brights_syscall_abi_init(void);
int64_t brights_syscall_handle(brights_trap_frame_t *tf);

#endif
