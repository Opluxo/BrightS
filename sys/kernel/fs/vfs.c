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

static void seed_dir(const char *path)
{
  brights_ramfs_stat_t st;
  if (brights_ramfs_stat(path, &st) == 0) {
    return;
  }
  brights_ramfs_mkdir(path);
}

void brights_vfs_init(void)
{
  brights_ramfs_init();
  
  // /sys - Core files (kernel)
  seed_dir("/sys");
  
  // /usr - User files
  seed_dir("/usr");
  seed_dir("/usr/home");
  seed_dir("/usr/home/root");
  seed_dir("/usr/home/guest");
  
  // /bin - Software packages
  seed_dir("/bin");
  seed_dir("/bin/pkg");
  seed_dir("/bin/config");
  seed_dir("/bin/config/root");
  seed_dir("/bin/config/guest");
  seed_dir("/bin/firmware");
  seed_dir("/bin/runtime");
  seed_dir("/bin/runtime/rust");
  seed_dir("/bin/runtime/c");
  seed_dir("/bin/runtime/python");
  
  // /mnt - Mount points
  seed_dir("/mnt");
  seed_dir("/mnt/drive");
  seed_dir("/mnt/input");
  seed_dir("/mnt/input/keyboard");
  seed_dir("/mnt/input/mouse");
  seed_dir("/mnt/input/touchpad");
  seed_dir("/mnt/input/camera");
  seed_dir("/mnt/input/biometric");
  seed_dir("/mnt/output");
  
  // /tmp - Cache files
  seed_dir("/tmp");
  
  // /swp - Swap partition
  seed_dir("/swp");
  
  // /dev - Device files
  seed_dir("/dev");
  
  // Seed system configuration files in /sys
  seed_file("/sys/init.rc", "echo BrightS userspace init\n");
  seed_file("/sys/profile", "USER=guest\nHOME=/usr/home\n");
  
  // Seed user configuration files in /bin/config
  seed_file("/bin/config/root/example.pf",
            "username:root\nhostname:brights\navatar:\"default\"\nemail:root@local\npassword:root\n");
  seed_file("/bin/config/guest/example.pf",
            "username:guest\nhostname:brights\navatar:\"default\"\nemail:guest@local\npassword:guest\n");
  
  // Seed user home files
  seed_file("/usr/home/readme.txt", "Welcome to /usr/home\n");
  seed_file("/usr/home/notes.txt", "");
  seed_file("/usr/home/root/readme.txt", "Home of root\n");
  seed_file("/usr/home/guest/readme.txt", "Home of guest\n");
  
  // Seed system info files
  seed_file("/sys/readme.txt", "BrightS kernel core files\n");
  seed_file("/bin/readme.txt", "Software packages directory\n");
  seed_file("/bin/pkg/readme.txt", "Installed packages\n");
  seed_file("/bin/config/readme.txt", "User software configurations\n");
  seed_file("/bin/firmware/readme.txt", "Firmware packages\n");
  seed_file("/bin/runtime/readme.txt", "Runtime environments and compilers\n");
  seed_file("/bin/runtime/rust/readme.txt", "Rust compiler and toolchain\n  - rustc: Rust compiler\n  - cargo: Rust package manager\n");
  seed_file("/bin/runtime/c/readme.txt", "C compiler and toolchain\n  - gcc: GNU Compiler Collection\n  - clang: LLVM C compiler\n  - make: Build automation tool\n");
  seed_file("/bin/runtime/python/readme.txt", "Python interpreter and packages\n  - python3: Python 3 interpreter\n  - pip: Python package manager\n");
  seed_file("/mnt/readme.txt", "Mount points directory\n");
  seed_file("/mnt/drive/readme.txt", "Mobile disk mounts\n");
  seed_file("/mnt/input/readme.txt", "Input devices\n");
  seed_file("/mnt/input/keyboard/readme.txt", "Keyboard device\n");
  seed_file("/mnt/input/mouse/readme.txt", "Mouse device\n");
  seed_file("/mnt/input/touchpad/readme.txt", "Touchpad device\n");
  seed_file("/mnt/input/camera/readme.txt", "Camera device\n");
  seed_file("/mnt/input/biometric/readme.txt", "Biometric input device\n");
  seed_file("/mnt/output/readme.txt", "Output devices\n");
  seed_file("/tmp/readme.txt", "Temporary cache files\n");
  seed_file("/swp/readme.txt", "Swap partition area\n");
}

int brights_vfs_mount_external(const char *backend)
{
  if (!backend || backend[0] == 0) {
    backend = "unknown";
  }

  int rc = brights_btrfs_mount();
  if (rc == 0) {
    seed_file("/mnt/drive/.mounted", "1\n");
    seed_file("/mnt/drive/fs", "btrfs\n");
    seed_file("/mnt/drive/role", "system\n");
    seed_file("/mnt/drive/backend", backend);
    seed_file("/mnt/drive/readme.txt",
              "System disk is mounted with Btrfs.\n");
  } else {
    seed_file("/mnt/drive/.mounted", "0\n");
    seed_file("/mnt/drive/fs", "none\n");
    seed_file("/mnt/drive/role", "system\n");
    seed_file("/mnt/drive/backend", backend);
    seed_file("/mnt/drive/readme.txt",
              "System disk mount failed (Btrfs required).\n");
  }
  return rc;
}
