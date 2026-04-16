#include "kernel/stddef.h"
#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void __chkstk(void);

void *memcpy(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

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

  if (n >= 256) {
    __asm__ __volatile__("rep movsb"
                         : "+D"(d), "+S"(s), "+c"(n)
                         :
                         : "memory");
    return dst;
  }

  size_t unalign = (8 - ((uintptr_t)d & 7)) & 7;
  if (unalign > 0) {
    *(uint64_t *)d = *(const uint64_t *)s;
    d += unalign;
    s += unalign;
    n -= unalign;
  }

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

  while (n >= 16) {
    ((uint64_t *)d)[0] = ((const uint64_t *)s)[0];
    ((uint64_t *)d)[1] = ((const uint64_t *)s)[1];
    d += 16;
    s += 16;
    n -= 16;
  }

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
  uint8_t byte = (uint8_t)c;
  
  if (n < 8) {
    for (size_t i = 0; i < n; ++i) d[i] = byte;
    return dst;
  }

  uint64_t pattern = byte;
  pattern |= pattern << 8;
  pattern |= pattern << 16;
  pattern |= pattern << 32;

  size_t unalign = (8 - ((uintptr_t)d & 7)) & 7;
  if (unalign > 0) {
    *(uint64_t *)d = pattern;
    d += unalign;
    n -= unalign;
  }

  while (n >= 64) {
    ((uint64_t *)d)[0] = pattern;
    ((uint64_t *)d)[1] = pattern;
    ((uint64_t *)d)[2] = pattern;
    ((uint64_t *)d)[3] = pattern;
    ((uint64_t *)d)[4] = pattern;
    ((uint64_t *)d)[5] = pattern;
    ((uint64_t *)d)[6] = pattern;
    ((uint64_t *)d)[7] = pattern;
    d += 64;
    n -= 64;
  }

  while (n >= 16) {
    ((uint64_t *)d)[0] = pattern;
    ((uint64_t *)d)[1] = pattern;
    d += 16;
    n -= 16;
  }

  if (n >= 8) {
    *(uint64_t *)d = pattern;
    d += 8;
    n -= 8;
  }

  for (size_t i = 0; i < n; ++i) d[i] = byte;
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const uint8_t *a = (const uint8_t *)s1;
  const uint8_t *b = (const uint8_t *)s2;

  while (n >= 8) {
    if (*(uint64_t *)a != *(uint64_t *)b) {
      for (size_t i = 0; i < 8; ++i) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
      }
    }
    a += 8;
    b += 8;
    n -= 8;
  }

  for (size_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

__attribute__((used)) void __chkstk(void)
{
  __asm__ __volatile__("" : : : "memory");
}
