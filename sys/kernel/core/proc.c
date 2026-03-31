#include "proc.h"

#define BRIGHTS_PROC_MAX 64u

static brights_proc_info_t proc_table[BRIGHTS_PROC_MAX];
static uint32_t next_pid = 1;
static uint32_t current_pid = 0;

// String copy helper
static void proc_str_copy(char *dst, int cap, const char *src)
{
  if (!dst || cap <= 0) return;
  int i = 0;
  while (src && src[i] && i < cap - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
}

void brights_proc_init(void)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    proc_table[i].pid = 0;
    proc_table[i].ppid = 0;
    proc_table[i].state = BRIGHTS_PROC_UNUSED;
    proc_table[i].name[0] = 0;
    proc_table[i].exit_code = 0;
  }
  next_pid = 1;
  current_pid = 0;
}

int brights_proc_spawn_kernel(const char *name)
{
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == BRIGHTS_PROC_UNUSED) {
      proc_table[i].pid = next_pid++;
      proc_table[i].ppid = current_pid;
      proc_table[i].state = BRIGHTS_PROC_RUNNABLE;
      proc_table[i].exit_code = 0;
      proc_str_copy(proc_table[i].name, BRIGHTS_PROC_NAME_LEN, name ? name : "kernel");
      return (int)proc_table[i].pid;
    }
  }
  return -1;
}

int brights_proc_set_state(uint32_t pid, brights_proc_state_t state)
{
  if (pid == 0) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      proc_table[i].state = state;
      return 0;
    }
  }
  return -1;
}

uint32_t brights_proc_count(brights_proc_state_t state)
{
  uint32_t count = 0;
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state == state) {
      ++count;
    }
  }
  return count;
}

uint32_t brights_proc_total(void)
{
  uint32_t count = 0;
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      ++count;
    }
  }
  return count;
}

int brights_proc_info_at(uint32_t index, brights_proc_info_t *info_out)
{
  if (!info_out || index >= BRIGHTS_PROC_MAX) {
    return -1;
  }
  *info_out = proc_table[index];
  return (info_out->state == BRIGHTS_PROC_UNUSED) ? -1 : 0;
}

int brights_proc_get_by_pid(uint32_t pid, brights_proc_info_t *info_out)
{
  if (!info_out || pid == 0) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      *info_out = proc_table[i];
      return 0;
    }
  }
  return -1;
}

int brights_proc_exit(uint32_t pid, int exit_code)
{
  if (pid == 0) {
    return -1;
  }
  for (uint32_t i = 0; i < BRIGHTS_PROC_MAX; ++i) {
    if (proc_table[i].pid == pid && proc_table[i].state != BRIGHTS_PROC_UNUSED) {
      proc_table[i].state = BRIGHTS_PROC_ZOMBIE;
      proc_table[i].exit_code = exit_code;
      return 0;
    }
  }
  return -1;
}

uint32_t brights_proc_current(void)
{
  return current_pid;
}

void brights_proc_set_current(uint32_t pid)
{
  current_pid = pid;
}
