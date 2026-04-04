#include "libc.h"

/* Simple find command for BrightS */

static void find_dir(const char *path, const char *name_pattern, int print_all)
{
  int fd = sys_open(path, 0);
  if (fd < 0) return;

  char buf[4096];
  int64_t n = sys_readdir(fd, buf, sizeof(buf) - 1);
  sys_close(fd);

  if (n <= 0) return;
  buf[n] = 0;

  /* Parse newline-separated entries */
  char *entry = buf;
  while (*entry) {
    char *end = entry;
    while (*end && *end != '\n') ++end;
    if (*end == '\n') { *end = 0; ++end; }

    if (entry[0] == 0) { entry = end; continue; }

    /* Build full path */
    char fullpath[256];
    int p = 0;
    const char *src = path;
    while (*src && p < 250) fullpath[p++] = *src++;
    if (fullpath[p - 1] != '/') fullpath[p++] = '/';
    int name_len = 0;
    while (entry[name_len] && p + name_len < 250) {
      fullpath[p + name_len] = entry[name_len];
      ++name_len;
    }
    fullpath[p + name_len] = 0;

    /* Check if name matches pattern */
    if (print_all || (name_pattern && strstr(entry, name_pattern))) {
      puts(fullpath);
    }

    /* Recurse into directories */
    uint64_t size = 0;
    uint32_t mode = 0;
    if (sys_stat(fullpath, &size, &mode) == 0) {
      if (mode & 0x4000) { /* S_IFDIR */
        find_dir(fullpath, name_pattern, print_all);
      }
    }

    entry = end;
  }
}

int main(int argc, char **argv)
{
  const char *path = ".";
  const char *name = 0;
  int print_all = 0;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'n' && i + 1 < argc) {
        name = argv[++i];
      } else {
        print_all = 1;
      }
    } else {
      path = argv[i];
    }
  }

  if (!name) print_all = 1;

  find_dir(path, name, print_all);
  return 0;
}
