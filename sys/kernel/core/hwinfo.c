#include "hwinfo.h"
#include "../platform/x86_64/io.h"
#include "../platform/x86_64/msr.h"
#include <stdint.h>

static brights_cpu_info_t cpu_info;
static int hwinfo_ready;

static void cpuid_leaf(uint32_t leaf, uint32_t subleaf,
                       uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  uint32_t a, b, c, d;
  __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(subleaf));
  if (eax) *eax = a;
  if (ebx) *ebx = b;
  if (ecx) *ecx = c;
  if (edx) *edx = d;
}

static inline uint64_t rdtsc_serialized(void)
{
  uint32_t lo, hi;
  /* cpuid serializes the instruction stream */
  __asm__ __volatile__("cpuid" : "=a"(lo) : "a"(0) : "rbx", "rcx", "rdx");
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtsc_now(void)
{
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

void brights_hwinfo_init(void)
{
  uint32_t eax, ebx, ecx, edx;

  /* Leaf 0: vendor string */
  cpuid_leaf(0, 0, &eax, &ebx, &ecx, &edx);
  cpu_info.max_basic_leaf = eax;
  ((uint32_t *)cpu_info.vendor)[0] = ebx;
  ((uint32_t *)cpu_info.vendor)[1] = edx;
  ((uint32_t *)cpu_info.vendor)[2] = ecx;
  cpu_info.vendor[12] = 0;

  /* Leaf 0x80000000: extended leaf count */
  cpuid_leaf(0x80000000, 0, &eax, 0, 0, 0);
  cpu_info.max_ext_leaf = eax;

  /* Leaf 0x80000002-4: brand string */
  if (cpu_info.max_ext_leaf >= 0x80000004) {
    uint32_t *brand = (uint32_t *)cpu_info.brand;
    for (uint32_t i = 0; i < 3; ++i) {
      cpuid_leaf(0x80000002 + i, 0, &brand[i*4], &brand[i*4+1], &brand[i*4+2], &brand[i*4+3]);
    }
    cpu_info.brand[48] = 0;
  } else {
    cpu_info.brand[0] = 0;
  }

  /* Leaf 1: family/model/stepping + features */
  cpuid_leaf(1, 0, &eax, &ebx, &ecx, &edx);
  cpu_info.stepping = eax & 0xF;
  cpu_info.model = (eax >> 4) & 0xF;
  cpu_info.family = (eax >> 8) & 0xF;
  if (cpu_info.family == 0x6 || cpu_info.family == 0xF) {
    cpu_info.model |= ((eax >> 16) & 0xF) << 4;
  }
  if (cpu_info.family == 0xF) {
    cpu_info.family += (eax >> 20) & 0xFF;
  }

  cpu_info.has_tsc   = (edx >> 4) & 1;
  cpu_info.has_msr   = (edx >> 5) & 1;
  cpu_info.has_apic  = (edx >> 9) & 1;
  cpu_info.has_sse   = (edx >> 25) & 1;
  cpu_info.has_sse2  = (edx >> 26) & 1;
  cpu_info.has_sse3  = (ecx >> 0) & 1;
  cpu_info.has_ssse3 = (ecx >> 9) & 1;
  cpu_info.has_sse41 = (ecx >> 19) & 1;
  cpu_info.has_sse42 = (ecx >> 20) & 1;
  cpu_info.has_x2apic= (ecx >> 21) & 1;
  cpu_info.has_aes   = (ecx >> 25) & 1;
  cpu_info.has_xsave = (ecx >> 26) & 1;
  cpu_info.has_avx   = (ecx >> 28) & 1;
  cpu_info.has_f16c  = (ecx >> 29) & 1;
  cpu_info.has_rdrand= (ecx >> 30) & 1;

  /* Logical core count from leaf 1 EBX */
  cpu_info.logical_cores = (ebx >> 16) & 0xFF;

  /* Leaf 7: extended features */
  if (cpu_info.max_basic_leaf >= 7) {
    cpuid_leaf(7, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.has_bmi1   = (ebx >> 3) & 1;
    cpu_info.has_avx2   = (ebx >> 5) & 1;
    cpu_info.has_smep   = (ebx >> 7) & 1;
    cpu_info.has_bmi2   = (ebx >> 8) & 1;
    cpu_info.has_avx512f= (ebx >> 16) & 1;
    cpu_info.has_smap   = (ebx >> 20) & 1;
    cpu_info.has_pku    = (ecx >> 3) & 1;
    cpu_info.has_umip   = (ecx >> 2) & 1;
  }

  /* Detect cache topology */
  /* Leaf 4: deterministic cache parameters (Intel) */
  cpu_info.l1d_size = 0; cpu_info.l1d_line = 64;
  cpu_info.l1i_size = 0;
  cpu_info.l2_size = 0;
  cpu_info.l3_size = 0;
  cpu_info.threads_per_core = 1;
  cpu_info.cores_per_pkg = 1;

  if (cpu_info.max_basic_leaf >= 4) {
    for (uint32_t i = 0; ; ++i) {
      cpuid_leaf(4, i, &eax, &ebx, &ecx, &edx);
      uint32_t type = eax & 0x1F;
      if (type == 0) break; /* No more caches */

      uint32_t line_size = (ebx & 0xFFF) + 1;
      uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
      uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
      uint32_t sets = ecx + 1;
      uint32_t cache_size = ways * partitions * line_size * sets;

      /* type: 1=data, 2=instruction, 3=unified */
      if (type == 1 && i == 0) {
        cpu_info.l1d_size = cache_size;
        cpu_info.l1d_line = line_size;
      } else if (type == 2 && cpu_info.l1i_size == 0) {
        cpu_info.l1i_size = cache_size;
      } else if (type == 3 && cache_size > cpu_info.l2_size && cache_size < 0x800000) {
        if (cpu_info.l2_size == 0) cpu_info.l2_size = cache_size;
        else cpu_info.l3_size = cache_size;
      }

      /* Max cores sharing this cache */
      uint32_t max_sharing = ((eax >> 14) & 0xFFF) + 1;
      if (type == 3 && max_sharing > cpu_info.cores_per_pkg) {
        cpu_info.cores_per_pkg = max_sharing;
      }
    }
  }

  /* Fallback cache sizes for Tiger Lake (i5-1135G7) if not detected */
  if (cpu_info.l1d_size == 0) cpu_info.l1d_size = 48 * 1024;
  if (cpu_info.l1i_size == 0) cpu_info.l1i_size = 32 * 1024;
  if (cpu_info.l2_size == 0)  cpu_info.l2_size  = 1280 * 1024;  /* 1.25 MB */
  if (cpu_info.l3_size == 0)  cpu_info.l3_size  = 8 * 1024 * 1024;
  if (cpu_info.cores_per_pkg <= 1) cpu_info.cores_per_pkg = 4;
  if (cpu_info.logical_cores <= 1) cpu_info.logical_cores = 8;

  /* Check invariant TSC */
  if (cpu_info.has_tsc && cpu_info.max_ext_leaf >= 0x80000007) {
    cpuid_leaf(0x80000007, 0, 0, 0, 0, &edx);
    cpu_info.tsc_invariant = (edx >> 8) & 1;
  }

  hwinfo_ready = 1;
}

const brights_cpu_info_t *brights_hwinfo_cpu(void)
{
  if (!hwinfo_ready) brights_hwinfo_init();
  return &cpu_info;
}

void brights_hwinfo_calibrate_tsc(void)
{
  /* Use PIT channel 2 to measure TSC frequency */
  /* Gate PIT channel 2 to speaker port */
  outb(0x61, (inb(0x61) & ~0x02) | 0x01); /* Disable speaker, enable gate */

  /* Program PIT channel 2: one-shot, lobyte/hibyte */
  outb(0x43, 0xB0); /* Channel 2, lobyte/hibyte, hardware retriggerable one-shot */

  /* Count down from 11932 (~10ms at 1193182 Hz) */
  uint16_t pit_count = 11932;
  outb(0x42, pit_count & 0xFF);
  outb(0x42, (pit_count >> 8) & 0xFF);

  /* Start counting */
  uint8_t port61 = inb(0x61);
  outb(0x61, port61 & ~1); /* Low */
  outb(0x61, port61 | 1);  /* High → triggers countdown */

  uint64_t tsc_start = rdtsc_now();

  /* Wait for PIT channel 2 output to go high (count expired) */
  while ((inb(0x61) & 0x20) == 0) {
    /* Spin */
  }

  uint64_t tsc_end = rdtsc_now();
  uint64_t tsc_delta = tsc_end - tsc_start;

  /* PIT count / PIT_FREQ = elapsed seconds */
  /* tsc_delta / elapsed = frequency */
  /* frequency = tsc_delta * PIT_FREQ / pit_count */
  /* PIT_FREQ = 1193182, pit_count = 11932 → elapsed ≈ 0.01s */
  /* freq = tsc_delta * 100 (since elapsed ≈ 1/100s) */
  cpu_info.tsc_freq = tsc_delta * 100;

  /* Sanity check */
  if (cpu_info.tsc_freq < 500000000ULL || cpu_info.tsc_freq > 6000000000ULL) {
    cpu_info.tsc_freq = 2400000000ULL; /* Fallback: 2.4 GHz */
  }
}
