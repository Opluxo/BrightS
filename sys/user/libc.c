#include "libc.h"

/* ===== String functions ===== */

void *memset(void *s, int c, uint64_t n)
{
  uint8_t *p = (uint8_t *)s;
  for (uint64_t i = 0; i < n; ++i) p[i] = (uint8_t)c;
  return s;
}

void *memcpy(void *dst, const void *src, uint64_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint64_t i = 0; i < n; ++i) d[i] = s[i];
  return dst;
}

void *memmove(void *dst, const void *src, uint64_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  if (d < s) {
    for (uint64_t i = 0; i < n; ++i) d[i] = s[i];
  } else {
    for (uint64_t i = n; i > 0; --i) d[i - 1] = s[i - 1];
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, uint64_t n)
{
  const uint8_t *a = (const uint8_t *)s1;
  const uint8_t *b = (const uint8_t *)s2;
  for (uint64_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

uint64_t strlen(const char *s)
{
  uint64_t n = 0;
  while (s[n]) ++n;
  return n;
}

char *strcpy(char *dst, const char *src)
{
  char *d = dst;
  while ((*d++ = *src++));
  return dst;
}

char *strncpy(char *dst, const char *src, uint64_t n)
{
  char *d = dst;
  while (n > 0 && *src) { *d++ = *src++; --n; }
  while (n > 0) { *d++ = 0; --n; }
  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s1 == *s2) { ++s1; ++s2; }
  return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, uint64_t n)
{
  while (n > 0 && *s1 && *s1 == *s2) { ++s1; ++s2; --n; }
  return n == 0 ? 0 : (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

char *strcat(char *dst, const char *src)
{
  char *d = dst;
  while (*d) ++d;
  while ((*d++ = *src++));
  return dst;
}

char *strchr(const char *s, int c)
{
  while (*s) {
    if (*s == (char)c) return (char *)s;
    ++s;
  }
  return 0;
}

char *strrchr(const char *s, int c)
{
  const char *last = 0;
  for (int i = 0; s[i]; ++i) {
    if (s[i] == (char)c) last = &s[i];
  }
  return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
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

/* ===== Additional string functions ===== */

int sscanf(const char *str, const char *format, ...)
{
  /* Very simplified implementation - only supports %[^]] for service name parsing */
  if (strstr(format, "%[^]]") && str[0] == '[') {
    /* Special case: %[^]] */
    char *dest = (char *)((void **)&format)[1]; /* Hack: get first vararg */

    const char *start = str + 1; /* Skip [ */
    const char *end = strchr(start, ']');
    if (end) {
      uint64_t len = end - start;
      if (len > 0 && len < 32) {
        memcpy(dest, start, len);
        dest[len] = 0;
        return 1;
      }
    }
  }
  return 0;
}

int atoi(const char *str)
{
  int num = 0;
  int neg = 0;
  const char *s = str;

  while (*s == ' ' || *s == '\t') s++;

  if (*s == '-') { neg = 1; s++; }
  else if (*s == '+') s++;

  while (*s >= '0' && *s <= '9') {
    num = num * 10 + (*s - '0');
    s++;
  }

  return neg ? -num : num;
}

static char *strtok_save = NULL;

char *strtok(char *str, const char *delim)
{
  if (str) {
    strtok_save = str;
  }

  if (!strtok_save) return NULL;

  /* Skip leading delimiters */
  char *start = strtok_save;
  while (*start && strchr(delim, *start)) start++;

  if (!*start) {
    strtok_save = NULL;
    return NULL;
  }

  /* Find end of token */
  char *end = start;
  while (*end && !strchr(delim, *end)) end++;

  if (*end) {
    *end = 0;
    strtok_save = end + 1;
  } else {
    strtok_save = NULL;
  }

  return start;
}

/* ===== Additional string functions ===== */

char *strndup(const char *s, uint64_t n)
{
  uint64_t len = strlen(s);
  if (n < len) len = n;

  char *dup = malloc(len + 1);
  if (dup) {
    memcpy(dup, s, len);
    dup[len] = 0;
  }
  return dup;
}

int vsnprintf(char *buf, uint64_t n, const char *fmt, ...)
{
  /* Very simplified implementation - only supports %s and %d */
  uint64_t pos = 0;
  const char *p = fmt;

  /* Get variable arguments (simplified) */
  void **args_ptr = (void **)&fmt + 1;
  int arg_index = 0;

  while (*p && pos < n - 1) {
    if (*p == '%') {
      p++;
      if (*p == 's') {
        char *str = (char *)args_ptr[arg_index++];
        while (*str && pos < n - 1) {
          buf[pos++] = *str++;
        }
      } else if (*p == 'd') {
        int num = (int)(int64_t)args_ptr[arg_index++];
        char numbuf[16];
        sprintf(numbuf, "%d", num);
        char *s = numbuf;
        while (*s && pos < n - 1) {
          buf[pos++] = *s++;
        }
      } else {
        buf[pos++] = *p;
      }
      p++;
    } else {
      buf[pos++] = *p++;
    }
  }

  buf[pos] = 0;
  return pos;
}

/*
 * Basic I/O functions
 */
int getchar(void)
{
    char c;
    if (sys_read(0, &c, 1) == 1) {
        return (int)(unsigned char)c;
    }
    return EOF;
}

int fflush(void *stream)
{
    (void)stream;
    /* For now, just return success */
    return 0;
}

/* ===== Minimal malloc (brk-based) ===== */

static uint64_t heap_start = 0;
static uint64_t heap_end = 0;
static uint64_t heap_brk = 0;

#define MALLOC_ALIGN 8
#define SERVICE_NAME_MAX 32

typedef struct {
  uint64_t size;  /* Size of this block (excluding header) */
  uint64_t free;  /* 1 if free, 0 if used */
} malloc_header_t;

void *malloc(uint64_t size)
{
  if (size == 0) return 0;

  /* Align size */
  size = (size + MALLOC_ALIGN - 1) & ~(MALLOC_ALIGN - 1);

  /* Initialize heap on first call */
  if (heap_start == 0) {
    /* Get initial brk */
    extern int64_t sys_brk(uint64_t addr);
    heap_start = (uint64_t)sys_brk(0);
    heap_end = heap_start;
    heap_brk = heap_start;

    /* Allocate initial 4KB heap */
    sys_brk(heap_start + 4096);
    heap_end = heap_start + 4096;

    /* Create initial free block */
    malloc_header_t *hdr = (malloc_header_t *)heap_start;
    hdr->size = 4096 - sizeof(malloc_header_t);
    hdr->free = 1;
  }

  /* Find first fit */
  uint64_t cur = heap_start;
  while (cur < heap_brk) {
    malloc_header_t *hdr = (malloc_header_t *)cur;
    if (hdr->free && hdr->size >= size) {
      /* Split block if there's enough room */
      uint64_t remaining = hdr->size - size;
      if (remaining > sizeof(malloc_header_t) + MALLOC_ALIGN) {
        malloc_header_t *new_hdr = (malloc_header_t *)(cur + sizeof(malloc_header_t) + size);
        new_hdr->size = remaining - sizeof(malloc_header_t);
        new_hdr->free = 1;
        hdr->size = size;
      }
      hdr->free = 0;
      return (void *)(cur + sizeof(malloc_header_t));
    }
    cur += sizeof(malloc_header_t) + hdr->size;
  }

  /* No free block found - extend heap */
  uint64_t extend = size + sizeof(malloc_header_t);
  if (extend < 4096) extend = 4096;

  extern int64_t sys_brk(uint64_t addr);
  uint64_t new_brk = heap_brk + extend;
  if (sys_brk(new_brk) == (int64_t)new_brk) {
    malloc_header_t *hdr = (malloc_header_t *)heap_brk;
    hdr->size = extend - sizeof(malloc_header_t);
    hdr->free = 0;
    heap_brk = new_brk;
    return (void *)(heap_brk - extend + sizeof(malloc_header_t));
  }

  return 0;
}

void free(void *ptr)
{
  if (!ptr) return;

  malloc_header_t *hdr = (malloc_header_t *)((uint64_t)ptr - sizeof(malloc_header_t));
  hdr->free = 1;

  /* Coalesce with next block if free */
  uint64_t next = (uint64_t)hdr + sizeof(malloc_header_t) + hdr->size;
  if (next < heap_brk) {
    malloc_header_t *next_hdr = (malloc_header_t *)next;
    if (next_hdr->free) {
      hdr->size += sizeof(malloc_header_t) + next_hdr->size;
    }
  }

  /* Coalesce with previous block if free */
  uint64_t cur = heap_start;
  malloc_header_t *prev = 0;
  while (cur < (uint64_t)hdr) {
    malloc_header_t *h = (malloc_header_t *)cur;
    uint64_t next_addr = cur + sizeof(malloc_header_t) + h->size;
    if (next_addr == (uint64_t)hdr && h->free) {
      h->size += sizeof(malloc_header_t) + hdr->size;
      break;
    }
    cur = next_addr;
  }
}

void *realloc(void *ptr, uint64_t new_size)
{
    if (!ptr) return malloc(new_size);
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    /* Get current block size */
    malloc_header_t *hdr = (malloc_header_t *)((uint64_t)ptr - sizeof(malloc_header_t));
    if (hdr->size >= new_size) return ptr; /* Block is big enough */

    /* Allocate new block and copy */
    void *new_ptr = malloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, hdr->size);
        free(ptr);
    }
    return new_ptr;
}

void *calloc(uint64_t nmemb, uint64_t size)
{
    uint64_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/* ===== Simple printf ===== */

static void print_char(int ch)
{
  extern int64_t sys_write(int fd, const void *buf, uint64_t count);
  char c = (char)ch;
  sys_write(1, &c, 1);
}

static void print_str(const char *s)
{
  while (*s) print_char(*s++);
}

static void print_u64(uint64_t v)
{
  char buf[24];
  int i = 0;
  if (v == 0) { print_char('0'); return; }
  while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
  while (i > 0) print_char(buf[--i]);
}

static void print_hex64(uint64_t v)
{
  static const char hex[] = "0123456789ABCDEF";
  print_str("0x");
  for (int i = 0; i < 16; ++i) {
    print_char(hex[(v >> (60 - i * 4)) & 0xF]);
  }
}

int printf(const char *fmt, ...)
{
  /* Simplified: only supports %s, %d, %u, %x, %c, %% */
  int count = 0;
  uint64_t *args = (uint64_t *)&fmt + 1;
  int arg_idx = 0;

  while (*fmt) {
    if (*fmt == '%') {
      ++fmt;
      switch (*fmt) {
        case 's': {
          const char *s = (const char *)args[arg_idx++];
          if (s) print_str(s); else print_str("(null)");
          break;
        }
        case 'd': {
          int64_t v = (int64_t)args[arg_idx++];
          if (v < 0) { print_char('-'); v = -v; }
          print_u64((uint64_t)v);
          break;
        }
        case 'u':
          print_u64(args[arg_idx++]);
          break;
        case 'x':
          print_hex64(args[arg_idx++]);
          break;
        case 'c':
          print_char((int)args[arg_idx++]);
          break;
        case '%':
          print_char('%');
          break;
        default:
          print_char('%');
          print_char(*fmt);
          break;
      }
    } else {
      print_char(*fmt);
    }
    ++fmt;
    ++count;
  }
  return count;
}

int puts(const char *s)
{
  print_str(s);
  print_char('\n');
  return 0;
}

int putchar(int ch)
{
  print_char(ch);
  return ch;
}

/* ===== System call wrappers ===== */

/* Syscall ABI: rax=syscall_number, rdi=a0, rsi=a1, rdx=a2, r10=a3, r8=a4, r9=a5 */

static inline int64_t syscall0(uint64_t num)
{
  int64_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num) : "memory");
  return ret;
}

static inline int64_t syscall1(uint64_t num, uint64_t a0)
{
  int64_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num), "D"(a0) : "memory");
  return ret;
}

static inline int64_t syscall2(uint64_t num, uint64_t a0, uint64_t a1)
{
  int64_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1) : "memory");
  return ret;
}

static inline int64_t syscall3(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2)
{
  int64_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1), "d"(a2) : "memory");
  return ret;
}

int64_t sys_read(int fd, void *buf, uint64_t count)
{
  return syscall3(1, (uint64_t)fd, (uint64_t)buf, count);
}

int64_t sys_write(int fd, const void *buf, uint64_t count)
{
  return syscall3(2, (uint64_t)fd, (uint64_t)buf, count);
}

int64_t sys_open(const char *path, int flags)
{
  return syscall2(3, (uint64_t)path, (uint64_t)flags);
}

int sys_close(int fd)
{
  return (int)syscall1(4, (uint64_t)fd);
}

int64_t sys_fork(void)
{
  return syscall0(6);
}

int64_t sys_exec(const char *path, char **argv, char **envp)
{
  return syscall3(7, (uint64_t)path, (uint64_t)argv, (uint64_t)envp);
}

int64_t sys_exit(int status)
{
  return syscall1(5, (uint64_t)status);
}

int64_t sys_getpid(void)
{
  return syscall0(9);
}

int64_t sys_getppid(void)
{
  return syscall0(10);
}

int64_t sys_wait(int pid, int *status)
{
  return syscall2(8, (uint64_t)pid, (uint64_t)status);
}

int64_t sys_sleep_ms(uint64_t ms)
{
  return syscall1(11, ms);
}

int64_t sys_clock_ms(void)
{
  return syscall0(13);
}

int64_t sys_mkdir(const char *path)
{
  return syscall1(14, (uint64_t)path);
}

int64_t sys_unlink(const char *path)
{
  return syscall1(15, (uint64_t)path);
}

int64_t sys_create(const char *path)
{
  return syscall1(16, (uint64_t)path);
}

int64_t sys_lseek(int fd, int64_t offset, int whence)
{
  return syscall3(17, (uint64_t)fd, (uint64_t)offset, (uint64_t)whence);
}

int64_t sys_stat(const char *path, uint64_t *size, uint32_t *mode)
{
  return syscall2(18, (uint64_t)path, (uint64_t)size);
}

int64_t sys_getcwd(char *buf, uint64_t size)
{
  return syscall2(30, (uint64_t)buf, size);
}

int64_t sys_chdir(const char *path)
{
  return syscall1(31, (uint64_t)path);
}

int64_t sys_readdir(int fd, char *buf, uint64_t count)
{
  return syscall3(32, (uint64_t)fd, (uint64_t)buf, count);
}

int64_t sys_yield(void)
{
  return syscall0(25);
}

int64_t sys_dup(int fd)
{
  return syscall1(27, (uint64_t)fd);
}

int64_t sys_dup2(int oldfd, int newfd)
{
  return syscall2(28, (uint64_t)oldfd, (uint64_t)newfd);
}

int64_t sys_pipe(int *fds)
{
  return syscall1(26, (uint64_t)fds);
}

int64_t sys_chmod(const char *path, uint32_t mode)
{
  return syscall2(48, (uint64_t)path, (uint64_t)mode);
}

int64_t sys_chown(const char *path, uint32_t uid, uint32_t gid)
{
  return syscall3(49, (uint64_t)path, (uint64_t)uid, (uint64_t)gid);
}

int64_t sys_symlink(const char *target, const char *linkpath)
{
  return syscall2(50, (uint64_t)target, (uint64_t)linkpath);
}

int64_t sys_readlink(const char *path, char *buf, uint64_t bufsize)
{
  return syscall3(51, (uint64_t)path, (uint64_t)buf, bufsize);
}

int64_t sys_uname(void *buf)
{
  return syscall1(38, (uint64_t)buf);
}

int64_t sys_sysinfo(void *buf)
{
  return syscall1(37, (uint64_t)buf);
}

int64_t sys_socket(int domain, int type, int protocol)
{
  return syscall3(52, (uint64_t)domain, (uint64_t)type, (uint64_t)protocol);
}

int64_t sys_bind(int sockfd, uint32_t addr, uint16_t port)
{
  return syscall3(53, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)port);
}

int64_t sys_listen(int sockfd, int backlog)
{
  return syscall2(54, (uint64_t)sockfd, (uint64_t)backlog);
}

int64_t sys_accept(int sockfd, uint32_t *addr, uint16_t *port)
{
  return syscall3(55, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)port);
}

int64_t sys_connect(int sockfd, uint32_t addr, uint16_t port)
{
  return syscall3(56, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)port);
}

int64_t sys_send(int sockfd, const void *buf, uint64_t len)
{
  return syscall3(57, (uint64_t)sockfd, (uint64_t)buf, len);
}

int64_t sys_recv(int sockfd, void *buf, uint64_t len)
{
  return syscall3(58, (uint64_t)sockfd, (uint64_t)buf, len);
}

int64_t sys_close_socket(int sockfd)
{
  return syscall1(59, (uint64_t)sockfd);
}

int64_t sys_icmp_echo(uint32_t dst_ip)
{
  return syscall1(62, (uint64_t)dst_ip);
}

int64_t sys_ifconfig(int cmd)
{
  return syscall1(61, (uint64_t)cmd);
}

int64_t sys_ip_parse(const char *str)
{
  return syscall1(63, (uint64_t)str);
}

void sys_ip_to_str(uint32_t ip, char *buf)
{
  syscall2(64, (uint64_t)ip, (uint64_t)buf);
}

void abort(void)
{
  sys_exit(1);
}

void exit(int status)
{
  sys_exit(status);
}
