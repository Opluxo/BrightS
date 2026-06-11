#include "libc.h"

/* Simple pwd command for BrightS */

int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  char buf[256];
  int64_t n = sys_getcwd(buf, sizeof(buf));
  if (n > 0) {
    puts(buf);
    return 0;
  }

  puts("/");
  return 0;
}
