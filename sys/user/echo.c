#include "libc.h"

/* Simple echo command for BrightS */

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    if (i > 1) putchar(' ');
    puts(argv[i]);
  }
  return 0;
}
