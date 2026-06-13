#ifndef BRIGHTS_KERNEL_UTIL_H
#define BRIGHTS_KERNEL_UTIL_H

#include <stdint.h>
#ifndef BRIGHTS_TEST_BUILD
#include "kernel/stddef.h"
#else
#include <stddef.h>
#endif

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

static inline char *kutil_strcat(char *dst, const char *src)
{
  char *d = dst;
  while (*d) ++d;
  while ((*d++ = *src++));
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

/* ===== IP address parsing ===== */

static inline int kutil_parse_ipv4(const char *str, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d)
{
    int count = 0;
    uint8_t vals[4] = {0};
    const char *p = str;
    
    while (*p && count < 4) {
        // Skip non-digits
        while (*p && (*p < '0' || *p > '9')) p++;
        
        if (!*p) break;
        
        // Parse number
        int val = 0;
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
            if (val > 255) return -1; // Invalid octet
        }
        
        vals[count++] = val;
        
        // Skip to next part
        while (*p && *p != '.') p++;
        if (*p == '.') p++;
    }
    
    if (count == 4) {
        if (a) *a = vals[0];
        if (b) *b = vals[1];
        if (c) *c = vals[2];
        if (d) *d = vals[3];
        return 4;
    }
    return -1;
}

/* ===== String tokenization ===== */

static inline char *kutil_strtok(char *str, const char *delim)
{
    static char *next_token = 0;
    
    if (str != NULL) {
        next_token = str;
    }
    
    if (next_token == NULL) {
        return NULL;
    }
    
    // Skip leading delimiters
    while (*next_token && kutil_strchr(delim, *next_token)) {
        next_token++;
    }
    
    if (*next_token == '\0') {
        next_token = NULL;
        return NULL;
    }
    
    // Find end of token
    char *token_start = next_token;
    while (*next_token && !kutil_strchr(delim, *next_token)) {
        next_token++;
    }
    
    if (*next_token != '\0') {
        *next_token = '\0';
        next_token++;
    } else {
        next_token = NULL;
    }
    
    return token_start;
}

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

/* Runtime functions */
void *memcpy(void *dst, const void *src, size_t n);
void __chkstk(void);

/* ===== UTF-8 utility functions ===== */

/* Decode UTF-8 sequence to Unicode codepoint.
   Returns the codepoint and sets *byte_len to the number of bytes consumed.
   Returns 0 on error with *byte_len = 1. */
static inline uint32_t kutil_utf8_decode(const char *str, int *byte_len)
{
  if (!str || !byte_len) { if (byte_len) *byte_len = 1; return 0; }
  unsigned char c = (unsigned char)str[0];

  if (c < 0x80) { *byte_len = 1; return c; }
  if ((c & 0xE0) == 0xC0) { *byte_len = 2; return ((uint32_t)(c & 0x1F) << 6) | (unsigned char)(str[1] & 0x3F); }
  if ((c & 0xF0) == 0xE0) { *byte_len = 3; return ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(str[1] & 0x3F) << 6) | (unsigned char)(str[2] & 0x3F); }
  if ((c & 0xF8) == 0xF0) { *byte_len = 4; return ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(str[1] & 0x3F) << 12) | ((uint32_t)(str[2] & 0x3F) << 6) | (unsigned char)(str[3] & 0x3F); }
  *byte_len = 1;
  return c;
}

/* Encode a Unicode codepoint to UTF-8. Returns number of bytes written (1-4). */
static inline int kutil_utf8_encode(uint32_t cp, char *out)
{
  if (cp < 0x80) { out[0] = (char)cp; return 1; }
  if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18));
  out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
}

/* Check if a byte is a UTF-8 continuation byte (0x80-0xBF). */
static inline int kutil_utf8_is_continuation(unsigned char c)
{
  return (c & 0xC0) == 0x80;
}

/* Get the expected byte length of a UTF-8 leading byte.
   Returns 0 if the byte is not a valid leading byte. */
static inline int kutil_utf8_lead_len(unsigned char c)
{
  if (c < 0x80) return 1;
  if ((c & 0xE0) == 0xC0) return 2;
  if ((c & 0xF0) == 0xE0) return 3;
  if ((c & 0xF8) == 0xF0) return 4;
  return 0;
}

/* Count the number of Unicode codepoints in a UTF-8 string. */
static inline int kutil_utf8_strlen(const char *str)
{
  if (!str) return 0;
  int count = 0;
  int pos = 0;
  while (str[pos]) {
    int len = kutil_utf8_lead_len((unsigned char)str[pos]);
    if (len == 0) len = 1;
    pos += len;
    count++;
  }
  return count;
}

/* Get display width of a codepoint (1 for ASCII, 2 for CJK, 0 for combining). */
static inline int kutil_codepoint_width(uint32_t cp)
{
  if (cp == 0) return 0;
  if (cp < 0x20) return 0;  /* Control chars */
  if (cp < 0x80) return 1;  /* ASCII */

  /* CJK Unified Ideographs and extensions */
  if ((cp >= 0x1100 && cp <= 0x115F) ||  /* Hangul Jamo */
      (cp >= 0x2E80 && cp <= 0xA4CF && cp != 0x303F) ||  /* CJK ... Yi */
      (cp >= 0xAC00 && cp <= 0xD7A3) ||  /* Hangul Syllables */
      (cp >= 0xF900 && cp <= 0xFAFF) ||  /* CJK Compatibility Ideographs */
      (cp >= 0xFE30 && cp <= 0xFE4F) ||  /* CJK Compatibility Forms */
      (cp >= 0xFF01 && cp <= 0xFF60) ||  /* Fullwidth Forms */
      (cp >= 0xFFE0 && cp <= 0xFFE6) ||  /* Fullwidth Signs */
      (cp >= 0x20000 && cp <= 0x2FA1F))  /* CJK Unified Ideographs Extension B+ */
    return 2;

  return 1;
}

/* Get display width of a UTF-8 string in terminal columns. */
static inline int kutil_utf8_strwidth(const char *str)
{
  if (!str) return 0;
  int width = 0;
  int pos = 0;
  while (str[pos]) {
    int len;
    uint32_t cp = kutil_utf8_decode(str + pos, &len);
    width += kutil_codepoint_width(cp);
    pos += len;
  }
  return width;
}

/* Check if a byte is a valid UTF-8 leading byte (not continuation). */
static inline int kutil_utf8_is_lead(unsigned char c)
{
  return c < 0x80 || (c >= 0xC0 && c <= 0xFD);
}

#endif
