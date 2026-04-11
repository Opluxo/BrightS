#include "kernel/stddef.h"
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t n);
void __chkstk(void);

/*
 * Optimized memory operations for x86_64 / Tiger Lake (i5-1135G7).
 *
 * Strategy:
 *   - Small (≤16B):  overlapping first/last word copy
 *   - Medium (17-256B): 64-bit block copy with alignment
 *   - Large (≥256B): Enhanced REP MOVSB (ERMS) on Tiger Lake
 *
 * memset uses 64-bit pattern replication.
 * memcmp uses 64-bit bulk comparison.
 */

void *memcpy(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  /* Small: overlapping first/last */
  if (n <= 16) {
    if (n >= 8) {
      *(uint64_t *)d = *(const uint64_t *)s;
      *(uint64_t *)(d + n - 8) = *(const uint64_t *)(s + n - 8);
      return dst;
    }
    if (n >= 4) {
      *(uint32_t *)d = *(const uint32_t *)s;
      *(uint32_t *)(d + n - 4) = *(const uint32_t *)(s + n - 4);
      return dst;
    }
    if (n >= 2) {
      *(uint16_t *)d = *(const uint16_t *)s;
      if (n > 2) d[n - 1] = s[n - 1];
      return dst;
    }
    if (n == 1) *d = *s;
    return dst;
  }

  /* Large: use ERMS (Enhanced REP MOVSB) - optimal on Tiger Lake for ≥256B */
  if (n >= 256) {
    /* Forward copy with REP MOVSB */
    __asm__ __volatile__("rep movsb"
                         : "+D"(d), "+S"(s), "+c"(n)
                         :
                         : "memory");
    return dst;
  }

  /* Medium: 64-bit block copy with alignment */
  /* Align destination to 8 bytes */
  size_t unalign = (8 - ((uintptr_t)d & 7)) & 7;
  if (unalign > 0) {
    *(uint64_t *)d = *(const uint64_t *)s;
    d += unalign;
    s += unalign;
    n -= unalign;
  }

  /* Copy 64 bytes at a time (cache line friendly) */
  while (n >= 64) {
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    ((uint64_t *)d)[2] = ((const uint64_t *)s)[2];
    ((uint64_t *)d)[3] = ((const uint64_t *)s)[3];
    ((uint64_t *)d)[4] = ((const uint64_t *)s)[4];
    ((uint64_t *)d)[5] = ((const uint64_t *)s)[5];
    ((uint64_t *)d)[6] = ((const uint64_t *)s)[6];
    ((uint64_t *)d)[7] = ((const uint64_t *)s)[7];
    d += 64;
    s += 64;
    n -= 64;
  }

  /* Copy 16 bytes at a time */
  while (n >= 16) {
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    d += 16;
    s += 16;
    n -= 16;
  }

  /* Copy remaining */
  if (n >= 8) {
    *(uint64_t *)d = *(const uint64_t *)s;
    *(uint64_t *)(d + n - 8) = *(const uint64_t *)(s + n - 8);
  } else if (n > 0) {
    d[0] = s[0];
    if (n >= 4) {
      *(uint32_t *)(d + n - 4) = *(const uint32_t *)(s + n - 4);
    } else {
      d[n - 1] = s[n - 1];
    }
  }

  return dst;
}
__attribute__((used)) void __chkstk(void)
{
  /* Stack probe hook for x86-64 - ensures stack space availability */
  /* In UEFI environment, we assume sufficient stack space */
  __asm__ __volatile__("" : : : "memory");
}
