#ifndef BRIGHTS_I386_SYSCALL_ABI_H
#define BRIGHTS_I386_SYSCALL_ABI_H

#include <stdint.h>
#include "trap.h"

void brights_syscall_abi_init(void);
int32_t brights_syscall_handle(brights_trap_frame_t *tf);

#endif
