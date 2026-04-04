#include <stdint.h>

#define BRIGHTS_PIPE_BUF_SIZE 4096u
#define BRIGHTS_MAX_PIPES 16

typedef struct {
  uint8_t buf[BRIGHTS_PIPE_BUF_SIZE];
  volatile uint32_t rd;
  volatile uint32_t wr;
  volatile uint32_t len;
  int read_end_open;
  int write_end_open;
} brights_pipe_t;

/* Pipe table */
static brights_pipe_t pipe_table[BRIGHTS_MAX_PIPES];
static int pipe_count = 0;

int brights_pipe_create(void)
{
  if (pipe_count >= BRIGHTS_MAX_PIPES) return -1;
  int idx = pipe_count++;
  brights_pipe_t *p = &pipe_table[idx];
  p->rd = 0;
  p->wr = 0;
  p->len = 0;
  p->read_end_open = 1;
  p->write_end_open = 1;
  return idx;
}

brights_pipe_t *brights_pipe_get(int idx)
{
  if (idx < 0 || idx >= pipe_count) return 0;
  return &pipe_table[idx];
}

int brights_pipe_write(brights_pipe_t *p, const uint8_t *src, uint32_t n)
{
  if (!p || !src || !p->write_end_open) return -1;
  uint32_t wrote = 0;
  while (wrote < n) {
    if (p->len >= BRIGHTS_PIPE_BUF_SIZE) break; /* Full */
    p->buf[p->wr] = src[wrote++];
    p->wr = (p->wr + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    ++p->len;
  }
  return (int)wrote;
}

int brights_pipe_read(brights_pipe_t *p, uint8_t *dst, uint32_t n)
{
  if (!p || !dst) return -1;
  uint32_t read = 0;
  while (read < n) {
    if (p->len == 0) {
      if (!p->write_end_open) break; /* EOF: writer closed */
      /* No data yet, return what we have */
      break;
    }
    dst[read++] = p->buf[p->rd];
    p->rd = (p->rd + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    --p->len;
  }
  return (int)read;
}

void brights_pipe_close_read(brights_pipe_t *p)
{
  if (p) p->read_end_open = 0;
}

void brights_pipe_close_write(brights_pipe_t *p)
{
  if (p) p->write_end_open = 0;
}
