#ifndef BRIGHTS_HWINFO_H
#define BRIGHTS_HWINFO_H

#include <stdint.h>

#define BRIGHTS_MAX_CPU_CORES 16

typedef struct {
  char vendor[13];
  char brand[49];
  uint32_t family;
  uint32_t model;
  uint32_t stepping;
  uint32_t max_basic_leaf;
  uint32_t max_ext_leaf;

  /* Feature flags (CPUID 1) */
  int has_apic;
  int has_x2apic;
  int has_tsc;
  int has_msr;
  int has_sse;
  int has_sse2;
  int has_sse3;
  int has_ssse3;
  int has_sse41;
  int has_sse42;
  int has_aes;
  int has_xsave;
  int has_avx;
  int has_f16c;
  int has_rdrand;

  /* Extended features (CPUID 7) */
  int has_avx2;
  int has_avx512f;
  int has_bmi1;
  int has_bmi2;
  int has_smep;
  int has_smap;
  int has_pku;
  int has_umip;

  /* Cache info */
  uint32_t l1d_size;      /* L1 data cache size in bytes */
  uint32_t l1d_line;      /* L1 data cache line size */
  uint32_t l1i_size;
  uint32_t l2_size;       /* L2 cache size in bytes */
  uint32_t l3_size;       /* L3 cache size in bytes */

  /* Topology */
  uint32_t logical_cores; /* Logical core count (from CPUID) */
  uint32_t cores_per_pkg; /* Physical cores per package */
  uint32_t threads_per_core;

  /* TSC */
  uint64_t tsc_freq;      /* Calibrated TSC frequency in Hz */
  int tsc_invariant;      /* Invariant TSC (constant rate) */
} brights_cpu_info_t;

void brights_hwinfo_init(void);
const brights_cpu_info_t *brights_hwinfo_cpu(void);

/* Calibrate TSC using PIT */
void brights_hwinfo_calibrate_tsc(void);

#endif
