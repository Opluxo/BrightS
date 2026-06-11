#include "libc.h"

/* Simple mv command for BrightS */

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("usage: mv <src> <dst>\n");
    return 1;
  }

  /* Check source exists */
  int fd = sys_open(argv[1], 0);
  if (fd < 0) {
    printf("mv: cannot stat '%s'\n", argv[1]);
    return 1;
  }
  sys_close(fd);

  /* Copy to destination */
  int src_fd = sys_open(argv[1], 0);
  if (src_fd < 0) {
    printf("mv: cannot open '%s'\n", argv[1]);
    return 1;
  }

  int dst_fd = sys_open(argv[2], 0x41); /* O_CREAT | O_WRONLY */
  if (dst_fd < 0) {
    printf("mv: cannot create '%s'\n", argv[2]);
    sys_close(src_fd);
    return 1;
  }

  char buf[512];
  int64_t n;
  while ((n = sys_read(src_fd, buf, sizeof(buf))) > 0) {
    sys_write(dst_fd, buf, (uint64_t)n);
  }

  sys_close(src_fd);
  sys_close(dst_fd);

  /* Remove source */
  if (sys_unlink(argv[1]) < 0) {
    printf("mv: cannot remove '%s'\n", argv[1]);
    return 1;
  }

  return 0;
}
