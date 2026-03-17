#include <stdint.h>

#define BRIGHTS_PIPE_BUF_SIZE 1024u

typedef struct {
  uint8_t buf[BRIGHTS_PIPE_BUF_SIZE];
  uint32_t rd;
  uint32_t wr;
  uint32_t len;
} brights_pipe_t;

void brights_pipe_init(brights_pipe_t *p)
{
  if (!p) {
    return;
  }
  p->rd = 0;
  p->wr = 0;
  p->len = 0;
}

int brights_pipe_write(brights_pipe_t *p, const uint8_t *src, uint32_t n)
{
  if (!p || !src) {
    return -1;
  }
  uint32_t wrote = 0;
  while (wrote < n && p->len < BRIGHTS_PIPE_BUF_SIZE) {
    p->buf[p->wr] = src[wrote++];
    p->wr = (p->wr + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    ++p->len;
  }
  return (int)wrote;
}

int brights_pipe_read(brights_pipe_t *p, uint8_t *dst, uint32_t n)
{
  if (!p || !dst) {
    return -1;
  }
  uint32_t read = 0;
  while (read < n && p->len > 0) {
    dst[read++] = p->buf[p->rd];
    p->rd = (p->rd + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    --p->len;
  }
  return (int)read;
}
