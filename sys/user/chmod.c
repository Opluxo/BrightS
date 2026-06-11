#include "libc.h"

/* Simple chmod command for BrightS */

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("usage: chmod <mode> <path>\n");
    return 1;
  }

  /* Parse octal mode */
  uint32_t mode = 0;
  const char *s = argv[1];
  while (*s >= '0' && *s <= '7') {
    mode = mode * 8 + (uint32_t)(*s - '0');
    ++s;
  }
  if (*s != 0) {
    printf("chmod: invalid mode '%s'\n", argv[1]);
    return 1;
  }

  if (sys_chmod(argv[2], mode) < 0) {
    printf("chmod: cannot change mode of '%s'\n", argv[2]);
    return 1;
  }

  return 0;
}
