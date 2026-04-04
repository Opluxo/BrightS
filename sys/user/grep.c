#include "libc.h"

/* Simple grep command for BrightS */

static void grep_file(const char *pattern, const char *filename)
{
  int fd = sys_open(filename, 0);
  if (fd < 0) {
    printf("grep: cannot open '%s'\n", filename);
    return;
  }

  char buf[4096];
  int64_t n = sys_read(fd, buf, sizeof(buf) - 1);
  sys_close(fd);

  if (n <= 0) return;
  buf[n] = 0;

  /* Find pattern in each line */
  char *line = buf;
  while (*line) {
    char *end = line;
    while (*end && *end != '\n') ++end;
    char saved = *end;
    *end = 0;

    if (strstr(line, pattern)) {
      if (filename) {
        printf("%s: %s\n", filename, line);
      } else {
        printf("%s\n", line);
      }
    }

    *end = saved;
    if (saved == 0) break;
    line = end + 1;
  }
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    printf("usage: grep <pattern> [file...]\n");
    return 1;
  }

  const char *pattern = argv[1];

  if (argc == 2) {
    /* Read from stdin */
    /* Simplified: just read and search */
    char buf[4096];
    int64_t n = sys_read(0, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = 0;
      char *line = buf;
      while (*line) {
        char *end = line;
        while (*end && *end != '\n') ++end;
        char saved = *end;
        *end = 0;
        if (strstr(line, pattern)) printf("%s\n", line);
        *end = saved;
        if (saved == 0) break;
        line = end + 1;
      }
    }
    return 0;
  }

  for (int i = 2; i < argc; ++i) {
    grep_file(pattern, argv[i]);
  }

  return 0;
}
