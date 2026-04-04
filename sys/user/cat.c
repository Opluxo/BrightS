#include "libc.h"

/* Simple cat command for BrightS */

static void cat_file(const char *path)
{
  int fd = sys_open(path, 0);
  if (fd < 0) {
    printf("cat: cannot open %s\n", path);
    return;
  }

  char buf[512];
  int64_t n;
  while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
    sys_write(1, buf, (uint64_t)n);
  }

  sys_close(fd);
}

int main(int argc, char **argv)
{
  if (argc <= 1) {
    /* Read from stdin */
    char buf[512];
    int64_t n;
    while ((n = sys_read(0, buf, sizeof(buf))) > 0) {
      sys_write(1, buf, (uint64_t)n);
    }
    return 0;
  }

  for (int i = 1; i < argc; ++i) {
    cat_file(argv[i]);
  }

  return 0;
}
