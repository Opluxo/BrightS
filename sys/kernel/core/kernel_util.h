#ifndef BRIGHTS_KERNEL_UTIL_H
#define BRIGHTS_KERNEL_UTIL_H

#include <stdint.h>

/* ===== Inline memory operations (fast, no libc dependency) ===== */

static inline void *kutil_memcpy(void *dst, const void *src, uint64_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  /* 8-byte aligned copy for performance */
  uint64_t qwords = n / 8;
  uint64_t *dq = (uint64_t *)d;
  const uint64_t *sq = (const uint64_t *)s;
  for (uint64_t i = 0; i < qwords; ++i) dq[i] = sq[i];

  /* Remaining bytes */
  uint64_t remain = n % 8;
  d += qwords * 8;
  s += qwords * 8;
  for (uint64_t i = 0; i < remain; ++i) d[i] = s[i];

  return dst;
}

static inline void *kutil_memset(void *s, int c, uint64_t n)
{
  uint8_t *p = (uint8_t *)s;
  uint8_t b = (uint8_t)c;

  /* 8-byte aligned set for performance */
  uint64_t qwords = n / 8;
  uint64_t pattern = (uint64_t)b * 0x0101010101010101ULL;
  uint64_t *qp = (uint64_t *)p;
  for (uint64_t i = 0; i < qwords; ++i) qp[i] = pattern;

  /* Remaining bytes */
  uint64_t remain = n % 8;
  p += qwords * 8;
  for (uint64_t i = 0; i < remain; ++i) p[i] = b;

  return s;
}

static inline void *kutil_memmove(void *dst, const void *src, uint64_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  if (d < s) {
    for (uint64_t i = 0; i < n; ++i) d[i] = s[i];
  } else if (d > s) {
    for (uint64_t i = n; i > 0; --i) d[i - 1] = s[i - 1];
  }
  return dst;
}

static inline int kutil_memcmp(const void *s1, const void *s2, uint64_t n)
{
  const uint8_t *a = (const uint8_t *)s1;
  const uint8_t *b = (const uint8_t *)s2;
  for (uint64_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

/* ===== Inline string operations ===== */

static inline uint64_t kutil_strlen(const char *s)
{
  uint64_t n = 0;
  while (s[n]) ++n;
  return n;
}

static inline char *kutil_strcpy(char *dst, const char *src)
{
  char *d = dst;
  while ((*d++ = *src++));
  return dst;
}

static inline int kutil_strcmp(const char *a, const char *b)
{
  while (*a && *b) {
    if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
    ++a; ++b;
  }
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline int kutil_strncmp(const char *a, const char *b, uint64_t n)
{
  while (n > 0 && *a && *b) {
    if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
    ++a; ++b; --n;
  }
  return n == 0 ? 0 : (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline char *kutil_strncpy(char *dst, const char *src, uint64_t n)
{
  char *d = dst;
  while (n > 0 && *src) { *d++ = *src++; --n; }
  while (n > 0) { *d++ = 0; --n; }
  return dst;
}

static inline char *kutil_strchr(const char *s, int c)
{
  while (*s) {
    if (*s == (char)c) return (char *)s;
    ++s;
  }
  return 0;
}

static inline char *kutil_strrchr(const char *s, int c)
{
  const char *last = 0;
  for (uint64_t i = 0; s[i]; ++i) {
    if (s[i] == (char)c) last = &s[i];
  }
  return (char *)last;
}

static inline char *kutil_strstr(const char *haystack, const char *needle)
{
  if (!*needle) return (char *)haystack;
  for (; *haystack; ++haystack) {
    if (*haystack == *needle) {
      const char *h = haystack;
      const char *n = needle;
      while (*n && *h == *n) { ++h; ++n; }
      if (!*n) return (char *)haystack;
    }
  }
  return 0;
}

/* ===== Number formatting ===== */

static inline int kutil_utoa(uint64_t val, char *buf, int base)
{
  static const char digits[] = "0123456789ABCDEF";
  char tmp[24];
  int i = 0;

  if (val == 0) { buf[0] = '0'; buf[1] = 0; return 1; }

  while (val > 0) {
    tmp[i++] = digits[val % base];
    val /= base;
  }

  int len = i;
  for (int j = 0; j < len; ++j) buf[j] = tmp[len - 1 - j];
  buf[len] = 0;
  return len;
}

static inline int kutil_itoa(int64_t val, char *buf)
{
  if (val < 0) {
    buf[0] = '-';
    return kutil_utoa((uint64_t)(-val), buf + 1, 10) + 1;
  }
  return kutil_utoa((uint64_t)val, buf, 10);
}

/* ===== Bit operations ===== */

static inline int kutil_ffs64(uint64_t x)
{
  if (x == 0) return -1;
  int n = 0;
  if (!(x & 0xFFFFFFFF)) { n += 32; x >>= 32; }
  if (!(x & 0xFFFF)) { n += 16; x >>= 16; }
  if (!(x & 0xFF)) { n += 8; x >>= 8; }
  if (!(x & 0xF)) { n += 4; x >>= 4; }
  if (!(x & 0x3)) { n += 2; x >>= 2; }
  if (!(x & 0x1)) { n += 1; }
  return n;
}

/* ===== Circular buffer ===== */

typedef struct {
  uint8_t *buf;
  uint64_t size;
  uint64_t rd;
  uint64_t wr;
  uint64_t count;
} kutil_ringbuf_t;

static inline void kutil_ringbuf_init(kutil_ringbuf_t *rb, uint8_t *buf, uint64_t size)
{
  rb->buf = buf;
  rb->size = size;
  rb->rd = 0;
  rb->wr = 0;
  rb->count = 0;
}

static inline int kutil_ringbuf_push(kutil_ringbuf_t *rb, uint8_t byte)
{
  if (rb->count >= rb->size) return -1;
  rb->buf[rb->wr] = byte;
  rb->wr = (rb->wr + 1) % rb->size;
  ++rb->count;
  return 0;
}

static inline int kutil_ringbuf_pop(kutil_ringbuf_t *rb, uint8_t *byte)
{
  if (rb->count == 0) return -1;
  *byte = rb->buf[rb->rd];
  rb->rd = (rb->rd + 1) % rb->size;
  --rb->count;
  return 0;
}

#endif
