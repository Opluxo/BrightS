#include "syscall_abi.h"
#include "msr.h"
#include "gdt.h"
#include "../../kernel/syscall.h"

#define IA32_EFER   0xC0000080u
#define IA32_STAR   0xC0000081u
#define IA32_LSTAR  0xC0000082u
#define IA32_SFMASK 0xC0000084u

extern void brights_syscall_entry(void);

void brights_syscall_abi_init(void)
{
  // Enable SYSCALL/SYSRET in EFER.
  uint64_t efer = rdmsr(IA32_EFER);
  efer |= 1u;
  wrmsr(IA32_EFER, efer);

  // STAR[47:32] = kernel CS, STAR[63:48] = user CS.
  uint64_t star = ((uint64_t)BRIGHTS_KERNEL_CS << 32) | ((uint64_t)BRIGHTS_USER_CS << 48);
  wrmsr(IA32_STAR, star);

  // LSTAR points to syscall entry.
  wrmsr(IA32_LSTAR, (uint64_t)(uintptr_t)brights_syscall_entry);

  // Mask IF on entry (optional); 0 leaves flags unchanged.
  wrmsr(IA32_SFMASK, 0);
}

int64_t brights_syscall_handle(brights_trap_frame_t *tf)
{
  if (!tf) {
    return -1;
  }
  return brights_syscall_dispatch(
    tf->rax, // syscall number
    tf->rdi,
    tf->rsi,
    tf->rdx,
    tf->rcx,
    tf->r8,
    tf->r9
  );
}
