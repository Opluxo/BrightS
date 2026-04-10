#ifndef BRIGHTS_PIPE_H
#define BRIGHTS_PIPE_H

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

/* Create a new pipe and return its index */
int brights_pipe_create(void);

/* Get a pipe by index */
brights_pipe_t *brights_pipe_get(int idx);

/* Write data to a pipe */
int brights_pipe_write(brights_pipe_t *p, const uint8_t *src, uint32_t n);

/* Read data from a pipe */
int brights_pipe_read(brights_pipe_t *p, uint8_t *dst, uint32_t n);

/* Close the read end of a pipe */
void brights_pipe_close_read(brights_pipe_t *p);

/* Close the write end of a pipe */
void brights_pipe_close_write(brights_pipe_t *p);

#endif