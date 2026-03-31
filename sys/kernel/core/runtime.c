#include <stddef.h>
#include <stdint.h>

// Optimized memory operations for x86_64
// Uses 64-bit operations when possible for better performance

void *memcpy(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  // Fast path for small copies
  if (n <= 16) {
    if (n >= 8) {
      // Copy first and last 8 bytes
      *(uint64_t *)d = *(const uint64_t *)s;
      *(uint64_t *)(d + n - 8) = *(const uint64_t *)(s + n - 8);
      return dst;
    }
    if (n >= 4) {
      // Copy first and last 4 bytes
      *(uint32_t *)d = *(const uint32_t *)s;
      *(uint32_t *)(d + n - 4) = *(const uint32_t *)(s + n - 4);
      return dst;
    }
    if (n >= 2) {
      *(uint16_t *)d = *(const uint16_t *)s;
      if (n > 2) {
        d[n - 1] = s[n - 1];
      }
      return dst;
    }
    if (n == 1) {
      *d = *s;
    }
    return dst;
  }

  // Align destination to 8 bytes
  size_t align = (8 - ((uintptr_t)d & 7)) & 7;
  if (align > 0 && align <= n) {
    for (size_t i = 0; i < align; ++i) {
      d[i] = s[i];
    }
    d += align;
    s += align;
    n -= align;
  }

  // Copy 32 bytes at a time
  while (n >= 32) {
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    ((uint64_t *)d)[2] = ((const uint64_t *)s)[2];
    ((uint64_t *)d)[3] = ((const uint64_t *)s)[3];
    d += 32;
    s += 32;
    n -= 32;
  }

  // Copy remaining 8 bytes at a time
  while (n >= 8) {
    *(uint64_t *)d = *(const uint64_t *)s;
    d += 8;
    s += 8;
    n -= 8;
  }

  // Copy remaining bytes
  while (n > 0) {
    *d++ = *s++;
    --n;
  }

  return dst;
}

void *memset(void *dst, int c, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  uint8_t v = (uint8_t)c;

  // Fast path for small fills
  if (n <= 16) {
    if (n >= 8) {
      uint64_t val = v * 0x0101010101010101ULL;
      *(uint64_t *)d = val;
      *(uint64_t *)(d + n - 8) = val;
      return dst;
    }
    if (n >= 4) {
      uint32_t val = v * 0x01010101U;
      *(uint32_t *)d = val;
      *(uint32_t *)(d + n - 4) = val;
      return dst;
    }
    if (n >= 2) {
      d[0] = v;
      d[n - 1] = v;
      return dst;
    }
    if (n == 1) {
      *d = v;
    }
    return dst;
  }

  // Build 64-bit pattern
  uint64_t pattern = v * 0x0101010101010101ULL;

  // Align destination to 8 bytes
  size_t align = (8 - ((uintptr_t)d & 7)) & 7;
  if (align > 0 && align <= n) {
    for (size_t i = 0; i < align; ++i) {
      d[i] = v;
    }
    d += align;
    n -= align;
  }

  // Fill 32 bytes at a time
  while (n >= 32) {
    ((uint64_t *)d)[0] = pattern;
    ((uint64_t *)d)[1] = pattern;
    ((uint64_t *)d)[2] = pattern;
    ((uint64_t *)d)[3] = pattern;
    d += 32;
    n -= 32;
  }

  // Fill remaining 8 bytes at a time
  while (n >= 8) {
    *(uint64_t *)d = pattern;
    d += 8;
    n -= 8;
  }

  // Fill remaining bytes
  while (n > 0) {
    *d++ = v;
    --n;
  }

  return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  if (d == s || n == 0) {
    return dst;
  }

  // Non-overlapping or forward copy
  if (d < s || d >= s + n) {
    return memcpy(dst, src, n);
  }

  // Backward copy for overlapping regions
  d += n;
  s += n;

  // Copy 32 bytes at a time backwards
  while (n >= 32) {
    d -= 32;
    s -= 32;
    n -= 32;
    ((uint64_t *)d)[3] = ((const uint64_t *)s)[3];
    ((uint64_t *)d)[2] = ((const uint64_t *)s)[2];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
  }

  // Copy remaining 8 bytes at a time
  while (n >= 8) {
    d -= 8;
    s -= 8;
    n -= 8;
    *(uint64_t *)d = *(const uint64_t *)s;
  }

  // Copy remaining bytes
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

  // Fast path for small comparisons
  if (n <= 8) {
    while (n > 0) {
      if (*pa != *pb) {
        return (int)*pa - (int)*pb;
      }
      ++pa;
      ++pb;
      --n;
    }
    return 0;
  }

  // Compare 8 bytes at a time
  while (n >= 8) {
    uint64_t va = *(const uint64_t *)pa;
    uint64_t vb = *(const uint64_t *)pb;
    if (va != vb) {
      // Find the differing byte
      for (int i = 0; i < 8; ++i) {
        if (pa[i] != pb[i]) {
          return (int)pa[i] - (int)pb[i];
        }
      }
    }
    pa += 8;
    pb += 8;
    n -= 8;
  }

  // Compare remaining bytes
  while (n > 0) {
    if (*pa != *pb) {
      return (int)*pa - (int)*pb;
    }
    ++pa;
    ++pb;
    --n;
  }

  return 0;
}

void __chkstk(void)
{
  // Minimal stack probe hook for the UEFI/COFF toolchain.
}
