#include "syscall_abi.h"
#include "../../kernel/core/syscall.h"
#include "../../kernel/core/sysent.h"

int32_t brights_syscall_handle(brights_trap_frame_t *tf)
{
  if (!tf) return -1;
  if (tf->eax >= (uint32_t)brights_sysent_count) return -38;
  return (int32_t)brights_syscall_dispatch(tf->eax, tf->ebx, tf->ecx, tf->edx, tf->esi, tf->edi, 0);
}

void brights_syscall_abi_init(void)
{
}
