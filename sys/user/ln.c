#include "libc.h"

/* Simple ln command for BrightS (symbolic links) */

int main(int argc, char **argv)
{
  int symbolic = 0;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-' && argv[i][1] == 's') {
      symbolic = 1;
    }
  }

  /* Find target and linkpath (last two non-option args) */
  const char *target = 0;
  const char *linkpath = 0;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {
      if (!target) target = argv[i];
      else linkpath = argv[i];
    }
  }

  if (!target || !linkpath) {
    printf("usage: ln [-s] <target> <linkpath>\n");
    return 1;
  }

  if (symbolic) {
    if (sys_symlink(target, linkpath) < 0) {
      printf("ln: cannot create symlink '%s'\n", linkpath);
      return 1;
    }
  } else {
    /* Hard link: copy file (simplified) */
    int src_fd = sys_open(target, 0);
    if (src_fd < 0) {
      printf("ln: cannot open '%s'\n", target);
      return 1;
    }

    int dst_fd = sys_open(linkpath, 0x41);
    if (dst_fd < 0) {
      printf("ln: cannot create '%s'\n", linkpath);
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
  }

  return 0;
}
