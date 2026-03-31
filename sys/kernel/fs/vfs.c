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
  
  // Seed Rust compiler scripts
  seed_file("/bin/runtime/rust/rustc",
    "#!/bin/sh\n"
    "# BrightS Rust Compiler Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-V\" ]; then\n"
    "  echo \"rustc 1.75.0 (BrightS 2024.01)\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ]; then\n"
    "  echo \"Usage: rustc [OPTIONS] INPUT\"\n"
    "  echo \"Options:\"\n"
    "  echo \"  -h, --help       Print help\"\n"
    "  echo \"  -V, --version    Print version\"\n"
    "  echo \"  -o, --output     Output file\"\n"
    "  echo \"  --edition        Rust edition (2015, 2018, 2021)\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] rustc: compiling...\"\n"
    "echo \"Note: Full compilation requires host rustc\"\n");
  
  seed_file("/bin/runtime/rust/cargo",
    "#!/bin/sh\n"
    "# BrightS Cargo Package Manager Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-V\" ]; then\n"
    "  echo \"cargo 1.75.0 (BrightS 2024.01)\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ] || [ -z \"$1\" ]; then\n"
    "  echo \"Rust's package manager\"\n"
    "  echo \"Usage: cargo [OPTIONS] [COMMAND]\"\n"
    "  echo \"Commands:\"\n"
    "  echo \"  new      Create a new cargo package\"\n"
    "  echo \"  build    Compile the current package\"\n"
    "  echo \"  run      Run a binary or example\"\n"
    "  echo \"  test     Run tests\"\n"
    "  echo \"  update   Update dependencies\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] cargo: $1\"\n");
  
  seed_file("/bin/runtime/rust/rustup",
    "#!/bin/sh\n"
    "# BrightS Rustup Toolchain Manager\n"
    "echo \"rustup 1.26.0 (BrightS 2024.01)\"\n"
    "echo \"Note: Toolchain management via host system\"\n");
  
  // Seed C compiler scripts
  seed_file("/bin/runtime/c/gcc",
    "#!/bin/sh\n"
    "# BrightS GCC Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-v\" ]; then\n"
    "  echo \"gcc (BrightS) 13.2.0\"\n"
    "  echo \"Copyright (C) 2023 Free Software Foundation, Inc.\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ]; then\n"
    "  echo \"Usage: gcc [OPTIONS] FILE...\"\n"
    "  echo \"Options:\"\n"
    "  echo \"  -h, --help       Print help\"\n"
    "  echo \"  -v, --version    Print version\"\n"
    "  echo \"  -o, --output     Output file\"\n"
    "  echo \"  -c               Compile only\"\n"
    "  echo \"  -Wall            Enable all warnings\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] gcc: compiling...\"\n"
    "echo \"Note: Full compilation requires host gcc\"\n");
  
  seed_file("/bin/runtime/c/clang",
    "#!/bin/sh\n"
    "# BrightS Clang Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-v\" ]; then\n"
    "  echo \"clang version 17.0.6 (BrightS 2024.01)\"\n"
    "  echo \"Target: x86_64-brights-unknown\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ]; then\n"
    "  echo \"Usage: clang [OPTIONS] FILE...\"\n"
    "  echo \"Options:\"\n"
    "  echo \"  -h, --help       Print help\"\n"
    "  echo \"  -v, --version    Print version\"\n"
    "  echo \"  -o, --output     Output file\"\n"
    "  echo \"  -c               Compile only\"\n"
    "  echo \"  -Wall            Enable all warnings\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] clang: compiling...\"\n"
    "echo \"Note: Full compilation requires host clang\"\n");
  
  seed_file("/bin/runtime/c/make",
    "#!/bin/sh\n"
    "# BrightS Make Wrapper\n"
    "if [ \"$1\" = \"--version\" ]; then\n"
    "  echo \"GNU Make 4.4.1 (BrightS 2024.01)\"\n"
    "  echo \"Built for x86_64-brights-unknown\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ -z \"$1\" ]; then\n"
    "  echo \"Usage: make [OPTIONS] [TARGET]\"\n"
    "  echo \"Options:\"\n"
    "  echo \"  -h, --help       Print help\"\n"
    "  echo \"  -v, --version    Print version\"\n"
    "  echo \"  -f, --file       Specify makefile\"\n"
    "  echo \"  -j, --jobs       Number of parallel jobs\"\n"
    "  echo \"  -C, --directory  Change directory\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] make: building...\"\n"
    "echo \"Note: Full build requires host make\"\n");
  
  seed_file("/bin/runtime/c/gdb",
    "#!/bin/sh\n"
    "# BrightS GDB Wrapper\n"
    "echo \"GNU gdb (BrightS) 14.1\"\n"
    "echo \"Note: Debugging via host gdb\"\n");
  
  // Seed Python interpreter scripts
  seed_file("/bin/runtime/python/python3",
    "#!/bin/sh\n"
    "# BrightS Python 3 Interpreter Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-V\" ]; then\n"
    "  echo \"Python 3.12.1 (BrightS 2024.01)\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ]; then\n"
    "  echo \"Usage: python3 [OPTIONS] [FILE] [ARG]...\"\n"
    "  echo \"Options:\"\n"
    "  echo \"  -h, --help       Print help\"\n"
    "  echo \"  -V, --version    Print version\"\n"
    "  echo \"  -c, --cmd        Execute command\"\n"
    "  echo \"  -m, --module     Run module\"\n"
    "  echo \"  -i, --interactive Interactive mode\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ -z \"$1\" ]; then\n"
    "  echo \"Python 3.12.1 (BrightS 2024.01)\"\n"
    "  echo \"Type \\\"help\\\", \\\"copyright\\\", \\\"credits\\\" for more information.\"\n"
    "  echo \"Note: Interactive mode via host python3\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] python3: running $1\"\n"
    "echo \"Note: Full execution requires host python3\"\n");
  
  seed_file("/bin/runtime/python/pip",
    "#!/bin/sh\n"
    "# BrightS Pip Package Manager Wrapper\n"
    "if [ \"$1\" = \"--version\" ] || [ \"$1\" = \"-V\" ]; then\n"
    "  echo \"pip 23.3.2 from /bin/runtime/python (python 3.12)\"\n"
    "  exit 0\n"
    "fi\n"
    "if [ \"$1\" = \"--help\" ] || [ \"$1\" = \"-h\" ] || [ -z \"$1\" ]; then\n"
    "  echo \"Usage: pip [OPTIONS] COMMAND\"\n"
    "  echo \"Commands:\"\n"
    "  echo \"  install    Install packages\"\n"
    "  echo \"  uninstall  Uninstall packages\"\n"
    "  echo \"  list       List installed packages\"\n"
    "  echo \"  show       Show package info\"\n"
    "  echo \"  search     Search PyPI\"\n"
    "  exit 0\n"
    "fi\n"
    "echo \"[BrightS] pip: $1\"\n");
  
  seed_file("/bin/runtime/python/pip3",
    "#!/bin/sh\n"
    "# BrightS Pip3 Package Manager Wrapper\n"
    "/bin/runtime/python/pip \"$@\"\n");
  
  seed_file("/bin/runtime/python/venv",
    "#!/bin/sh\n"
    "# BrightS Python Virtual Environment\n"
    "echo \"Python venv module (BrightS 2024.01)\"\n"
    "echo \"Note: Virtual environments via host python3\"\n");
  
  // Seed BrightS Package Manager (bspm) - installer
  seed_file("/bin/pkg/bspm",
    "#!/bin/sh\n"
    "# BrightS Package Manager (BSPM) Installer\n"
    "BSPM_REPO=\"https://github.com/s12mcOvO/BrightS_Package_Manager\"\n"
    "BSPM_VERSION=\"1.0.0\"\n"
    "\n"
    "case \"$1\" in\n"
    "  version|--version|-v)\n"
    "    echo \"BSPM Installer v$BSPM_VERSION\"\n"
    "    echo \"Repository: $BSPM_REPO\"\n"
    "    ;;\n"
    "  help|--help|-h|\"\")\n"
    "    echo \"BrightS Package Manager Installer\"\n"
    "    echo \"Usage: bspm [COMMAND]\"\n"
    "    echo \"Commands:\"\n"
    "    echo \"  install    Install BSPM from repository\"\n"
    "    echo \"  version    Show version\"\n"
    "    echo \"  help       Show this help\"\n"
    "    ;;\n"
    "  install)\n"
    "    echo \"[BSPM] Installing from $BSPM_REPO\"\n"
    "    echo \"[BSPM] Please clone the repository manually:\"\n"
    "    echo \"  git clone $BSPM_REPO\"\n"
    "    ;;\n"
    "  *)\n"
    "    echo \"Error: Unknown command '$1'\"\n"
    "    echo \"Run 'bspm help' for usage information\"\n"
    "    exit 1\n"
    "    ;;\n"
    "esac\n");
  
  seed_dir("/bin/pkg/.db");
  seed_file("/bin/pkg/readme.txt",
    "BrightS Package Manager (BSPM)\n"
    "Repository: https://github.com/s12mcOvO/BrightS_Package_Manager\n"
    "\n"
    "To install BSPM:\n"
    "  git clone https://github.com/s12mcOvO/BrightS_Package_Manager\n"
    "  cp BrightS_Package_Manager/bin/bspm /bin/pkg/\n"
    "\n"
    "Usage: bspm [command] [package]\n"
    "Commands: install, remove, list, search, update, upgrade\n");
  
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
