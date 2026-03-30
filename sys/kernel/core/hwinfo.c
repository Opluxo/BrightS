#include "hwinfo.h"
#include <stdint.h>

static brights_cpu_info_t cpu_info;
static int hwinfo_ready;

static void cpuid_leaf(uint32_t leaf, uint32_t subleaf,
                       uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  __asm__ __volatile__("cpuid"
                       : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                       : "a"(leaf), "c"(subleaf));
  if (eax) *eax = a;
  if (ebx) *ebx = b;
  if (ecx) *ecx = c;
  if (edx) *edx = d;
}

void brights_hwinfo_init(void)
{
  uint32_t eax = 0;
  uint32_t ebx = 0;
  uint32_t ecx = 0;
  uint32_t edx = 0;

  cpuid_leaf(0, 0, &eax, &ebx, &ecx, &edx);
  cpu_info.max_basic_leaf = eax;
  ((uint32_t *)cpu_info.vendor)[0] = ebx;
  ((uint32_t *)cpu_info.vendor)[1] = edx;
  ((uint32_t *)cpu_info.vendor)[2] = ecx;
  cpu_info.vendor[12] = 0;

  cpuid_leaf(1, 0, &eax, &ebx, &ecx, &edx);
  cpu_info.stepping = eax & 0xFu;
  cpu_info.model = (eax >> 4) & 0xFu;
  cpu_info.family = (eax >> 8) & 0xFu;
  if (cpu_info.family == 0x6u || cpu_info.family == 0xFu) {
    cpu_info.model |= ((eax >> 16) & 0xFu) << 4;
  }
  if (cpu_info.family == 0xFu) {
    cpu_info.family += (eax >> 20) & 0xFFu;
  }

  cpu_info.has_tsc = (edx >> 4) & 1u;
  cpu_info.has_msr = (edx >> 5) & 1u;
  cpu_info.has_apic = (edx >> 9) & 1u;
  cpu_info.has_sse = (edx >> 25) & 1u;
  cpu_info.has_sse2 = (edx >> 26) & 1u;
  cpu_info.has_sse3 = (ecx >> 0) & 1u;
  cpu_info.has_ssse3 = (ecx >> 9) & 1u;
  cpu_info.has_sse41 = (ecx >> 19) & 1u;
  cpu_info.has_sse42 = (ecx >> 20) & 1u;
  cpu_info.has_x2apic = (ecx >> 21) & 1u;
  cpu_info.has_aes = (ecx >> 25) & 1u;
  cpu_info.has_xsave = (ecx >> 26) & 1u;
  cpu_info.has_avx = (ecx >> 28) & 1u;

  hwinfo_ready = 1;
}

const brights_cpu_info_t *brights_hwinfo_cpu(void)
{
  if (!hwinfo_ready) {
    brights_hwinfo_init();
  }
  return &cpu_info;
}
