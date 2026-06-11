#ifndef BRIGHTS_I386_TRAP_H
#define BRIGHTS_I386_TRAP_H

#include <stdint.h>

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t vec;
    uint32_t err;
    uint32_t eip, cs, eflags, user_esp, ss;
} brights_trap_frame_t;

void brights_trap_handler(brights_trap_frame_t *tf);

#endif
