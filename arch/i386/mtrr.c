#include "mtrr.h"
#include "msr.h"
#include "../../drivers/serial.h"

/* MTRR MSRs */
#define IA32_MTRR_CAP       0x0FE
#define IA32_MTRR_DEF_TYPE  0x2FF
#define IA32_MTRR_PHYSBASE  0x200  /* + 2*index */
#define IA32_MTRR_PHYSMASK  0x201  /* + 2*index */

/* PAT MSR */
#define IA32_PAT            0x277

static int mtrr_var_count = 0;
static int mtrr_next_free = 0;

void brights_mtrr_init(void)
{
  /* Read MTRR capability */
  uint64_t cap = rdmsr(IA32_MTRR_CAP);
  mtrr_var_count = (int)(cap & 0xFF);

  /* Enable MTRRs and set default type to WB */
  uint64_t def_type = rdmsr(IA32_MTRR_DEF_TYPE);
  def_type |= (1ULL << 11); /* E (MTRR enable) */
  def_type &= ~0xFFULL;
  def_type |= MTRR_WB;       /* Default: Write Back */
  wrmsr(IA32_MTRR_DEF_TYPE, def_type);
}

int brights_mtrr_set(uint64_t phys_base, uint64_t size, uint8_t type)
{
  if (mtrr_next_free >= mtrr_var_count) return -1;

  /* Size must be a power of 2 */
  if (size == 0 || (size & (size - 1)) != 0) return -1;

  /* Base must be aligned to size */
  if (phys_base & (size - 1)) return -1;

  int idx = mtrr_next_free++;

  /* Set base: type in low bits, address shifted right by 12 */
  uint64_t base_val = (phys_base & 0x000FFFFFFFFFF000ULL) | (type & 0xFF);
  wrmsr(IA32_MTRR_PHYSBASE + idx * 2, base_val);

  /* Set mask: valid bit + address mask */
  uint64_t mask_val = (~(size - 1) & 0x000FFFFFFFFFF000ULL) | (1ULL << 11); /* Valid */
  wrmsr(IA32_MTRR_PHYSMASK + idx * 2, mask_val);

  return idx;
}

void brights_mtrr_clear(int index)
{
  if (index < 0 || index >= mtrr_var_count) return;
  wrmsr(IA32_MTRR_PHYSBASE + index * 2, 0);
  wrmsr(IA32_MTRR_PHYSMASK + index * 2, 0);
}

int brights_mtrr_count(void)
{
  return mtrr_var_count;
}

/* Configure PAT for custom page caching */
void brights_pat_init(void)
{
  /*
   * PAT layout (8 entries, 8 bits each):
   *   Entry 0: WB (0x06)
   *   Entry 1: WT (0x04)
   *   Entry 2: UC- (0x07)
   *   Entry 3: UC (0x00)
   *   Entry 4: WC (0x01)
   *   Entry 5: WP (0x05)
   *   Entry 6: WB (0x06)
   *   Entry 7: UC (0x00)
   */
  uint64_t pat = 0;
  pat |= (uint64_t)0x06 << (0 * 8);  /* 0: WB */
  pat |= (uint64_t)0x04 << (1 * 8);  /* 1: WT */
  pat |= (uint64_t)0x07 << (2 * 8);  /* 2: UC- */
  pat |= (uint64_t)0x00 << (3 * 8);  /* 3: UC */
  pat |= (uint64_t)0x01 << (4 * 8);  /* 4: WC */
  pat |= (uint64_t)0x05 << (5 * 8);  /* 5: WP */
  pat |= (uint64_t)0x06 << (6 * 8);  /* 6: WB */
  pat |= (uint64_t)0x00 << (7 * 8);  /* 7: UC */
  wrmsr(IA32_PAT, pat);

  /* Flush TLB to apply new PAT */
  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
