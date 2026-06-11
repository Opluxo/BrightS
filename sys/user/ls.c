#include "libc.h"

/* Simple ls command for BrightS */

static void list_dir(const char *path, int long_format)
{
  int fd = sys_open(path, 0);
  if (fd < 0) {
    printf("ls: cannot open %s\n", path);
    return;
  }

  char buf[4096];
  int64_t n = sys_readdir(fd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    sys_close(fd);
    return;
  }
  buf[n] = 0;
  sys_close(fd);

  /* Parse newline-separated entries */
  char *entry = buf;
  while (*entry) {
    char *end = entry;
    while (*end && *end != '\n') ++end;
    if (*end == '\n') { *end = 0; ++end; }

    if (entry[0] == 0) { entry = end; continue; }

    if (long_format) {
      /* Build full path for stat */
      char fullpath[256];
      int p = 0;
      while (*path && p < 250) fullpath[p++] = *path++;
      if (fullpath[p - 1] != '/') fullpath[p++] = '/';
      int name_len = 0;
      while (entry[name_len] && p + name_len < 250) {
        fullpath[p + name_len] = entry[name_len];
        ++name_len;
      }
      fullpath[p + name_len] = 0;

      uint64_t size = 0;
      uint32_t mode = 0;
      if (sys_stat(fullpath, &size, &mode) == 0) {
        /* Print mode bits */
        if (mode & 0x4000) putchar('d');
        else if (mode & 0xA000) putchar('l');
        else putchar('-');
        putchar(mode & 0x100 ? 'r' : '-');
        putchar(mode & 0x080 ? 'w' : '-');
        putchar(mode & 0x040 ? 'x' : '-');
        putchar(mode & 0x020 ? 'r' : '-');
        putchar(mode & 0x010 ? 'w' : '-');
        putchar(mode & 0x008 ? 'x' : '-');
        putchar(mode & 0x004 ? 'r' : '-');
        putchar(mode & 0x002 ? 'w' : '-');
        putchar(mode & 0x001 ? 'x' : '-');
        printf(" %8u ", (uint32_t)size);
      }
    }

    puts(entry);
    entry = end;
  }
}

int main(int argc, char **argv)
{
  int long_format = 0;
  const char *dir = ".";

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      for (int j = 1; argv[i][j]; ++j) {
        if (argv[i][j] == 'l') long_format = 1;
      }
    } else {
      dir = argv[i];
    }
  }

  list_dir(dir, long_format);
  return 0;
}
