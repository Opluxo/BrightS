#include "vfs.h"
#include "ramfs.h"
#include "btrfs.h"
#include <stdint.h>

static void seed_file(const char *path, const char *content)
{
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    fd = brights_ramfs_create(path);
  }
  if (fd < 0) {
    return;
  }
  if (!content) {
    return;
  }
  const char *p = content;
  uint64_t len = 0;
  while (p[len]) {
    ++len;
  }
  brights_ramfs_write(fd, 0, content, len);
}

void brights_vfs_init(void)
{
  brights_ramfs_init();
  seed_file("/config/userspace/init.rc", "echo BrightS userspace init\n");
  seed_file("/config/userspace/profile", "USER=guest\nHOME=/usr/home\n");
  seed_file("/config/root/example.pf",
            "username:root\nhostname:brights\navatar:\"default\"\nemail:root@local\npassword:root\n");
  seed_file("/config/guest/example.pf",
            "username:guest\nhostname:brights\navatar:\"default\"\nemail:guest@local\npassword:guest\n");
  seed_file("/usr/home/readme.txt", "Welcome to /usr/home\n");
  seed_file("/usr/home/notes.txt", "");
  seed_file("/usr/home/root/readme.txt", "Home of root\n");
  seed_file("/usr/home/guest/readme.txt", "Home of guest\n");
}

int brights_vfs_mount_external(const char *backend)
{
  if (!backend || backend[0] == 0) {
    backend = "unknown";
  }

  int rc = brights_btrfs_mount();
  if (rc == 0) {
    seed_file("/dev/mnt/.mounted", "1\n");
    seed_file("/dev/mnt/fs", "btrfs\n");
    seed_file("/dev/mnt/role", "system\n");
    seed_file("/dev/mnt/backend", backend);
    seed_file("/dev/mnt/readme.txt",
              "System disk is mounted with Btrfs.\n");
  } else {
    seed_file("/dev/mnt/.mounted", "0\n");
    seed_file("/dev/mnt/fs", "none\n");
    seed_file("/dev/mnt/role", "system\n");
    seed_file("/dev/mnt/backend", backend);
    seed_file("/dev/mnt/readme.txt",
              "System disk mount failed (Btrfs required).\n");
  }
  return rc;
}
