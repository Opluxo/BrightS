#include "libc.h"

/* Simple rm command for BrightS */

int main(int argc, char **argv)
{
  if (argc <= 1) {
    printf("usage: rm <path> [path...]\n");
    return 1;
  }

  int ret = 0;
  for (int i = 1; i < argc; ++i) {
    if (sys_unlink(argv[i]) < 0) {
      printf("rm: cannot remove '%s'\n", argv[i]);
      ret = 1;
    }
  }

  return ret;
}
