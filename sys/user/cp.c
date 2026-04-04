#include "libc.h"

/* Simple cp command for BrightS */

static int copy_file(const char *src, const char *dst)
{
  int src_fd = sys_open(src, 0);
  if (src_fd < 0) {
    printf("cp: cannot open '%s'\n", src);
    return -1;
  }

  int dst_fd = sys_open(dst, 0x41); /* O_CREAT | O_WRONLY */
  if (dst_fd < 0) {
    printf("cp: cannot create '%s'\n", dst);
    sys_close(src_fd);
    return -1;
  }

  char buf[512];
  int64_t n;
  while ((n = sys_read(src_fd, buf, sizeof(buf))) > 0) {
    sys_write(dst_fd, buf, (uint64_t)n);
  }

  sys_close(src_fd);
  sys_close(dst_fd);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("usage: cp <src> <dst>\n");
    return 1;
  }

  return copy_file(argv[1], argv[2]) < 0 ? 1 : 0;
}
