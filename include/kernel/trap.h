#ifndef BRIGHTS_X86_64_TRAP_H
#define BRIGHTS_X86_64_TRAP_H

#include <stdint.h>

typedef struct {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t vec;
  uint64_t err;
  uint64_t rip, cs, rflags, rsp, ss;
} brights_trap_frame_t;

void brights_trap_handler(brights_trap_frame_t *tf);

#endif
