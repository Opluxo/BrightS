#include "cpu_local.h"
#include "msr.h"

#define IA32_KERNEL_GS_BASE 0xC0000102u

typedef struct {
  uint64_t kernel_rsp;
} brights_cpu_local_t;

static brights_cpu_local_t cpu0;

void brights_cpu_local_init(uint64_t kernel_rsp)
{
  cpu0.kernel_rsp = kernel_rsp;
  wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)(uintptr_t)&cpu0);
}
