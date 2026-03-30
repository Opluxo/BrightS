#ifndef BRIGHTS_HWINFO_H
#define BRIGHTS_HWINFO_H

#include <stdint.h>

typedef struct {
  char vendor[13];
  uint32_t family;
  uint32_t model;
  uint32_t stepping;
  uint32_t max_basic_leaf;
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
} brights_cpu_info_t;

void brights_hwinfo_init(void);
const brights_cpu_info_t *brights_hwinfo_cpu(void);

#endif
