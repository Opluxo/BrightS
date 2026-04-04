#include <stddef.h>
#include <stdint.h>

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

void *memset(void *dst, int c, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  uint8_t v = (uint8_t)c;

  /* Small */
  if (n <= 16) {
    if (n >= 8) {
      uint64_t pat = v * 0x0101010101010101ULL;
      *(uint64_t *)d = pat;
      *(uint64_t *)(d + n - 8) = pat;
      return dst;
    }
    if (n >= 4) {
      uint32_t pat = v * 0x01010101U;
      *(uint32_t *)d = pat;
      *(uint32_t *)(d + n - 4) = pat;
      return dst;
    }
    if (n >= 2) { d[0] = v; d[n - 1] = v; return dst; }
    if (n == 1) *d = v;
    return dst;
  }

  uint64_t pat = v * 0x0101010101010101ULL;

  /* Align to 8 bytes */
  size_t unalign = (8 - ((uintptr_t)d & 7)) & 7;
  if (unalign > 0) {
    *(uint64_t *)d = pat;
    d += unalign;
    n -= unalign;
  }

  /* Fill 64 bytes at a time */
  while (n >= 64) {
    ((uint64_t *)d)[0] = pat;
    ((uint64_t *)d)[1] = pat;
    ((uint64_t *)d)[2] = pat;
    ((uint64_t *)d)[3] = pat;
    ((uint64_t *)d)[4] = pat;
    ((uint64_t *)d)[5] = pat;
    ((uint64_t *)d)[6] = pat;
    ((uint64_t *)d)[7] = pat;
    d += 64;
    n -= 64;
  }

  /* Fill 16 bytes at a time */
  while (n >= 16) {
    ((uint64_t *)d)[0] = pat;
    ((uint64_t *)d)[1] = pat;
    d += 16;
    n -= 16;
  }

  /* Fill remaining with overlap */
  if (n >= 8) {
    *(uint64_t *)d = pat;
    *(uint64_t *)(d + n - 8) = pat;
  } else {
    d[0] = v;
    d[n - 1] = v;
  }

  return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  if (d == s || n == 0) return dst;

  /* Forward copy */
  if (d < s || d >= s + n) {
    return memcpy(dst, src, n);
  }

  /* Backward copy */
  d += n;
  s += n;

  while (n >= 64) {
    d -= 64; s -= 64; n -= 64;
    ((uint64_t *)d)[7] = ((const uint64_t *)s)[7];
    ((uint64_t *)d)[6] = ((const uint64_t *)s)[6];
    ((uint64_t *)d)[5] = ((const uint64_t *)s)[5];
    ((uint64_t *)d)[4] = ((const uint64_t *)s)[4];
    ((uint64_t *)d)[3] = ((const uint64_t *)s)[3];
    ((uint64_t *)d)[2] = ((const uint64_t *)s)[2];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
  }

  while (n >= 8) {
    d -= 8; s -= 8; n -= 8;
    *(uint64_t *)d = *(const uint64_t *)s;
  }

  while (n > 0) {
    *--d = *--s;
    --n;
  }

  return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
  const uint8_t *pa = (const uint8_t *)a;
  const uint8_t *pb = (const uint8_t *)b;

  if (n <= 8) {
    while (n > 0) {
      if (*pa != *pb) return (int)*pa - (int)*pb;
      ++pa; ++pb; --n;
    }
    return 0;
  }

  /* Compare 8 bytes at a time */
  while (n >= 8) {
    uint64_t va = *(const uint64_t *)pa;
    uint64_t vb = *(const uint64_t *)pb;
    if (va != vb) {
      /* Use BSWAP to find first differing byte */
      uint64_t diff = va ^ vb;
      /* Find lowest set bit byte position */
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      if (!(diff & 0xFF)) { diff >>= 8; pa++; pb++; }
      return (int)*pa - (int)*pb;
    }
    pa += 8;
    pb += 8;
    n -= 8;
  }

  while (n > 0) {
    if (*pa != *pb) return (int)*pa - (int)*pb;
    ++pa; ++pb; --n;
  }

  return 0;
}

/* String length optimized for cache line reads */
size_t strlen(const char *s)
{
  const char *p = s;

  /* Align to 8 bytes */
  while (((uintptr_t)p & 7) && *p) ++p;
  if (!*p) return (size_t)(p - s);

  /* Check 8 bytes at a time */
  const uint64_t *wp = (const uint64_t *)p;
  for (;;) {
    uint64_t w = *wp;
    /* Check if any byte is zero: subtract 0x0101... and AND with 0x8080... */
    if (((w - 0x0101010101010101ULL) & ~w & 0x8080808080808080ULL)) {
      /* Found a zero byte */
      const char *cp = (const char *)wp;
      for (int i = 0; i < 8; ++i) {
        if (!cp[i]) return (size_t)(cp - s) + i;
      }
    }
    ++wp;
  }
}

/* String compare */
int strcmp(const char *a, const char *b)
{
  while (*a && *a == *b) { ++a; ++b; }
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
  while (n > 0 && *a && *a == *b) { ++a; ++b; --n; }
  if (n == 0) return 0;
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char *strcpy(char *dst, const char *src)
{
  char *d = dst;
  while ((*d++ = *src++));
  return dst;
}

void __chkstk(void)
{
  /* Minimal stack probe hook for UEFI/COFF toolchain */
}
