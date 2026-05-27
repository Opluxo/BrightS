#ifndef BRIGHTS_USERINIT_H
#define BRIGHTS_USERINIT_H

#include <stdint.h>

/* Initialize VFS2 and user mode infrastructure */
void brights_userinit(void);

/* Enter user mode with embedded/shell init process */
void brights_userinit_enter(void);

/* Enter user mode from syscall (for exec) */
void brights_enter_user_from_syscall(void *user_rip, void *user_rsp, uint64_t user_cr3);

#endif
