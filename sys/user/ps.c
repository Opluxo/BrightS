#include "libc.h"

/* Simple ps command for BrightS */

typedef struct {
  uint32_t pid;
  uint32_t ppid;
  uint32_t state;
  char name[32];
  int exit_code;
} proc_info_t;

static const char *state_name(uint32_t state)
{
  switch (state) {
    case 0: return "unused";
    case 1: return "runnable";
    case 2: return "running";
    case 3: return "sleeping";
    case 4: return "zombie";
    default: return "unknown";
  }
}

int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  printf("  PID   PPID  STATE      NAME\n");
  printf("----- ----- ---------- --------------------------------\n");

  for (uint32_t i = 0; i < 64; ++i) {
    proc_info_t info;
    /* Use sysinfo to get process count, then iterate */
    /* For now, we'll just print what we can get */
    int64_t pid = sys_getpid();
    if ((uint32_t)pid == i) {
      printf("%5u %5u %-10s %s\n",
             (uint32_t)pid,
             (uint32_t)sys_getppid(),
             state_name(2), /* running */
             "current");
    }
  }

  return 0;
}
