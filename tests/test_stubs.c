#include <stdint.h>
#include <stddef.h>

extern void *malloc(size_t);
extern void free(void *);

void *brights_pmem_alloc_page(void)
{
  void *p = malloc(4096);
  if (p) __builtin_memset(p, 0, 4096);
  return p;
}

typedef struct {
  char vendor[13];
  char brand[49];
  uint32_t family;
  uint32_t model;
  uint32_t stepping;
  uint32_t max_basic_leaf;
  uint32_t max_ext_leaf;
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
  int has_avx2;
  int has_avx512f;
  int has_bmi1;
  int has_bmi2;
  int has_smep;
  int has_smap;
  int has_pku;
  int has_umip;
  uint32_t l1d_size;
  uint32_t l1d_line;
  uint32_t l1i_size;
  uint32_t l2_size;
  uint32_t l3_size;
  uint32_t logical_cores;
  uint32_t cores_per_pkg;
  uint32_t threads_per_core;
  uint64_t tsc_freq;
  int tsc_invariant;
} brights_cpu_info_t;

const brights_cpu_info_t *brights_hwinfo_cpu(void)
{
  return 0;
}
