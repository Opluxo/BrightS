#include "libc.h"

/* Simple mkdir command for BrightS */

int main(int argc, char **argv)
{
  if (argc <= 1) {
    printf("usage: mkdir <path> [path...]\n");
    return 1;
  }

  int ret = 0;
  for (int i = 1; i < argc; ++i) {
    if (sys_mkdir(argv[i]) < 0) {
      printf("mkdir: cannot create directory '%s'\n", argv[i]);
      ret = 1;
    }
  }

  return ret;
}
