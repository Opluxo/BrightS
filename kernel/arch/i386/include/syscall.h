#ifndef BRIGHTS_KERNEL_ARCH_I386_SYSCALL_H
#define BRIGHTS_KERNEL_ARCH_I386_SYSCALL_H

#include <stdint.h>

#define BRIGHTS_I386_SYSCALL_MAX 128

typedef struct {
    uint32_t eax, edx, ecx, ebx, esi, edi;
} brights_i386_syscall_args_t;

typedef int (*brights_i386_syscall_handler_t)(brights_i386_syscall_args_t *args);

void brights_i386_syscall_init(void);
int brights_i386_syscall_register(int num, brights_i386_syscall_handler_t handler);
int brights_i386_syscall(int num, brights_i386_syscall_args_t *args);

extern void brights_i386_syscall_entry(void);

#endif