#ifndef BRIGHTS_SYSENT_H
#define BRIGHTS_SYSENT_H

#include <stdint.h>

typedef int64_t (*brights_syscall_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

typedef struct {
  uint8_t argc;
  brights_syscall_fn fn;
} brights_sysent_t;

extern brights_sysent_t brights_sysent_table[];
extern const uint64_t brights_sysent_count;

#endif
#include "sysent.h"
#include "../fs/ramfs.h"
#include "../fs/vfs.h"
#include "../fs/vfs2.h"
#include "../ipc/pipe.h"
#include "../net/net.h"
#include "elf.h"
#include "clock.h"
#include "sleep.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "kmalloc.h"
#include "pmem.h"
#include "syshook.h"
#include "../dev/serial.h"
#include "../dev/tty.h"

/* Syscall handler macros: reduce repetitive (void) casts */
#define SYSCALL0(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; body }

#define SYSCALL1(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ (void)a1;(void)a2;(void)a3;(void)a4;(void)a5; body }

#define SYSCALL2(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ (void)a2;(void)a3;(void)a4;(void)a5; body }

#define SYSCALL3(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ (void)a3;(void)a4;(void)a5; body }

#define SYSCALL5(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ (void)a5; body }

#define SYSCALL6(name, body) \
static int64_t sys_##name(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) \
{ body }

// Helper for string comparison
static int str_compare(const char *a, const char *b)
{
  while (*a && *b && *a == *b) { ++a; ++b; }
  return *a - *b;
}

SYSCALL0(nosys, { return -1; })

// sys_read(fd, buf, len) - Read from file descriptor
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  if (fd == 0) {
    // stdin - read from keyboard
    char *dst = (char *)(uintptr_t)buf;
    for (uint64_t i = 0; i < len; ++i) {
      dst[i] = brights_tty_read_char_blocking();
    }
    return (int64_t)len;
  }
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  return brights_vfs2_read(f, (void *)(uintptr_t)buf, len);
}

// sys_write(fd, buf, len) - Write to file descriptor
static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  if (fd == 1 || fd == 2) {
    // stdout/stderr - write to console
    const char *src = (const char *)(uintptr_t)buf;
    for (uint64_t i = 0; i < len; ++i) {
      char ch[2] = {src[i], 0};
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, ch);
    }
    return (int64_t)len;
  }
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  return brights_vfs2_write(f, (const void *)(uintptr_t)buf, len);
}

// sys_open(path, flags) - Open file
static int64_t sys_open(uint64_t path, uint64_t flags, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;

  /* Resolve path against cwd */
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) {
    return -1;
  }

  vfs_file_t *f = 0;
  if (brights_vfs2_open(resolved, (uint32_t)flags, &f) != 0) {
    return -1;
  }

  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) {
    brights_vfs2_close(f);
    return -1;
  }
  return brights_vfs2_fd_alloc(ft, f);
}

// sys_close(fd) - Close file descriptor
static int64_t sys_close(uint64_t fd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  brights_vfs2_close(f);
  brights_vfs2_fd_free(ft, (int)fd);
  return 0;
}

// sys_exit(status) - Exit process
static int64_t sys_exit(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  brights_proc_exit(brights_proc_current(), (int)status);
  brights_sched_yield();
  return 0;
}

// sys_getpid() - Get process ID
static int64_t sys_getpid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_proc_current();
}

// sys_getppid() - Get parent process ID
static int64_t sys_getppid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  brights_proc_info_t info;
  if (brights_proc_get_by_pid(brights_proc_current(), &info) == 0) {
    return (int64_t)info.ppid;
  }
  return -1;
}

// sys_fork() - Fork process
static int64_t sys_fork(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

  int child_pid = brights_proc_fork();
  if (child_pid <= 0) return -1;

  /* Add child to scheduler */
  brights_sched_add_process((uint32_t)child_pid);
  return child_pid;
}

// sys_exec(path, argv, envp) - Execute program
static int64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)argv; (void)envp; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;

  /* Resolve path */
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) {
    /* Try absolute path */
    int i = 0;
    while (pathname[i] && i < 255) { resolved[i] = pathname[i]; ++i; }
    resolved[i] = 0;
  }

  /* Try to read the file from ramfs */
  extern int brights_ramfs_open(const char *path);
  extern int64_t brights_ramfs_read(int fd, uint64_t offset, void *buf, uint64_t count);

  int fd = brights_ramfs_open(resolved);
  if (fd < 0) {
    /* Try /bin/ prefix */
    char binpath[256];
    int i = 0;
    const char *prefix = "/bin/";
    while (prefix[i]) { binpath[i] = prefix[i]; ++i; }
    int j = 0;
    while (resolved[j] && i < 250) { binpath[i++] = resolved[j++]; }
    binpath[i] = 0;
    fd = brights_ramfs_open(binpath);
  }
  if (fd < 0) return -1;

  /* Read the ELF file */
  static uint8_t elf_buf[65536];
  int64_t n = brights_ramfs_read(fd, 0, elf_buf, sizeof(elf_buf));
  if (n <= 0) return -1;

  /* Load ELF */
  extern int brights_elf_load(const void *buf, uint64_t size, elf64_load_info_t *info_out);
  elf64_load_info_t load_info;
  if (brights_elf_load(elf_buf, (uint64_t)n, &load_info) != 0) return -1;

  /* Jump to new entry point (no return from exec) */
  extern void brights_enter_user(void *rip, void *rsp);
  brights_enter_user((void *)(uintptr_t)load_info.entry,
                     (void *)(uintptr_t)load_info.stack_top);

  /* Should not reach here */
  return -1;
}

// sys_sleep_ms(ms) - Sleep for milliseconds
static int64_t sys_sleep_ms(uint64_t ms, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  brights_sleep_ms(ms);
  return 0;
}

// sys_clock_ns() - Get current time in nanoseconds
static int64_t sys_clock_ns(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_clock_ns();
}

// sys_clock_ms() - Get current time in milliseconds
static int64_t sys_clock_ms(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_clock_ms();
}

// sys_mkdir(path) - Create directory
static int64_t sys_mkdir(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  return brights_vfs2_mkdir(resolved);
}

// sys_unlink(path) - Remove file
static int64_t sys_unlink(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  return brights_vfs2_unlink(resolved);
}

// sys_create(path) - Create file
static int64_t sys_create(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  return brights_vfs2_create(resolved);
}

// sys_lseek(fd, offset, whence) - Seek in file
static int64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  return brights_vfs2_lseek(f, (int64_t)offset, (int)whence);
}

// sys_stat(path, statbuf) - Get file status
static int64_t sys_stat(uint64_t path, uint64_t statbuf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  uint64_t size = 0;
  uint32_t mode = 0;
  if (brights_vfs2_stat(resolved, &size, &mode) != 0) return -1;
  if (statbuf) {
    struct { uint64_t size; uint32_t mode; uint32_t pad; } *st = (void *)(uintptr_t)statbuf;
    st->size = size;
    st->mode = mode;
  }
  return 0;
}

// sys_mmap(addr, length, prot, flags, fd, offset) - Map memory
static int64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset)
{
  (void)addr; (void)prot; (void)flags; (void)fd; (void)offset;
  void *ptr = brights_kmalloc(length);
  if (ptr) {
    return (int64_t)(uintptr_t)ptr;
  }
  return -1;
}

// sys_munmap(addr, length) - Unmap memory
static int64_t sys_munmap(uint64_t addr, uint64_t length, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)length; (void)a2; (void)a3; (void)a4; (void)a5;
  brights_kfree((void *)(uintptr_t)addr);
  return 0;
}

// sys_brk(addr) - Set program break
static int64_t sys_brk(uint64_t addr, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  uint32_t pid = brights_proc_current();
  if (addr == 0) {
    /* Query current brk */
    uint64_t cur = brights_proc_get_brk(pid);
    if (cur == 0) cur = addr;
    return (int64_t)cur;
  }
  brights_proc_set_brk(pid, addr);
  return (int64_t)addr;
}

// sys_signal_raise(signo) - Raise signal
static int64_t sys_signal_raise(uint64_t signo, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_signal_raise((uint32_t)signo);
}

// sys_signal_pending() - Get pending signals
static int64_t sys_signal_pending(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_signal_pending();
}

// sys_signal_handler(handler) - Set signal handler
static int64_t sys_signal_handler(uint64_t handler, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  // Simplified - store handler address
  return handler;
}

// sys_yield() - Yield CPU to next process
static int64_t sys_yield(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_sched_yield();
}

// sys_wait(pid, status) - Wait for child process
static int64_t sys_wait(uint64_t pid, uint64_t status, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  /* Simple wait: poll until child becomes zombie */
  brights_proc_info_t info;
  for (;;) {
    if (brights_proc_get_by_pid((uint32_t)pid, &info) != 0) {
      return -1; /* No such process */
    }
    if (info.state == BRIGHTS_PROC_ZOMBIE) {
      if (status) {
        int *st = (int *)(uintptr_t)status;
        *st = info.exit_code;
      }
      return (int64_t)pid;
    }
    brights_sched_yield();
  }
}

// sys_pipe(fildes) - Create pipe
static int64_t sys_pipe(uint64_t fildes, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  int *fds = (int *)(uintptr_t)fildes;
  if (!fds) return -1;

  /* Create a pipe */

  int pipe_idx = brights_pipe_create();
  if (pipe_idx < 0) return -1;

  /* Allocate two file descriptors: read end and write end */
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;

  /* For now, store pipe index as fd numbers */
  /* fd[0] = read end, fd[1] = write end */
  int read_fd = -1, write_fd = -1;
  for (int i = 3; i < BRIGHTS_PROC_MAX_FDS; i += 2) {
    if (!ft[i] && !ft[i + 1]) {
      read_fd = i;
      write_fd = i + 1;
      break;
    }
  }
  if (read_fd < 0) return -1;

  /* Mark fds as used (pipe backing stored as index) */
  ft[read_fd] = (vfs_file_t *)(uintptr_t)(pipe_idx + 0x1000); /* read marker */
  ft[write_fd] = (vfs_file_t *)(uintptr_t)(pipe_idx + 0x2000); /* write marker */

  fds[0] = read_fd;
  fds[1] = write_fd;
  return 0;
}

// sys_dup(fd) - Duplicate file descriptor
static int64_t sys_dup(uint64_t fd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  /* Find a free fd */
  for (int i = 0; i < BRIGHTS_PROC_MAX_FDS; ++i) {
    if (!ft[i]) {
      ft[i] = f;
      return i;
    }
  }
  return -1;
}

// sys_dup2(fd1, fd2) - Duplicate file descriptor to specific fd
static int64_t sys_dup2(uint64_t fd1, uint64_t fd2, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd1);
  if (!f) return -1;
  /* Close fd2 if open */
  if (ft[(int)fd2]) {
    brights_vfs2_close(ft[(int)fd2]);
  }
  ft[(int)fd2] = f;
  return (int64_t)fd2;
}

// sys_ioctl(fd, request, argp) - Control device
static int64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t argp, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f || !f->fops || !f->fops->ioctl) return -1;
  return f->fops->ioctl(f, request, (void *)(uintptr_t)argp);
}

// sys_getcwd(buf, size) - Get current working directory
static int64_t sys_getcwd(uint64_t buf, uint64_t size, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  char *dst = (char *)(uintptr_t)buf;
  if (!dst || size == 0) return -1;
  const char *cwd = brights_proc_get_cwd();
  if (!cwd) return -1;
  uint64_t i = 0;
  while (cwd[i] && i < size - 1) {
    dst[i] = cwd[i];
    ++i;
  }
  dst[i] = 0;
  return (int64_t)i;
}

// sys_chdir(path) - Change directory
static int64_t sys_chdir(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  /* Verify directory exists */
  uint64_t size = 0;
  uint32_t mode = 0;
  if (brights_vfs2_stat(resolved, &size, &mode) != 0) return -1;
  if ((mode & VFS_S_IFDIR) == 0) return -1;
  return brights_proc_set_cwd(resolved);
}

// sys_readdir(fd, buf, count) - Read directory entries
static int64_t sys_readdir(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)fd);
  if (!f) return -1;
  return brights_vfs2_readdir("/", (char *)(uintptr_t)buf, count);
}

// sys_mount(source, target, fstype, flags, data) - Mount filesystem
static int64_t sys_mount(uint64_t source, uint64_t target, uint64_t fstype, uint64_t flags, uint64_t data, uint64_t a5)
{
  (void)source; (void)target; (void)fstype; (void)flags; (void)data; (void)a5;
  return brights_vfs_mount_external("manual");
}

// sys_umount(target) - Unmount filesystem
static int64_t sys_umount(uint64_t target, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)target; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return 0;
}

// sys_reboot(cmd) - Reboot system
static int64_t sys_reboot(uint64_t cmd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)cmd; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  // Triple fault to reboot
  __asm__ __volatile__("ud2");
  return 0;
}

// sys_shutdown() - Shutdown system
static int64_t sys_shutdown(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  // ACPI shutdown
  __asm__ __volatile__("hlt");
  return 0;
}

// sys_sysinfo(buf) - Get system information
static int64_t sys_sysinfo(uint64_t buf, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  struct {
    uint64_t uptime;
    uint64_t total_ram;
    uint64_t free_ram;
    uint64_t processes;
  } *info = (void *)(uintptr_t)buf;
  
  if (!info) {
    return -1;
  }
  
  info->uptime = brights_clock_ms() / 1000;
  info->total_ram = brights_pmem_total_bytes();
  info->free_ram = brights_pmem_free_bytes();
  info->processes = brights_proc_total();
  return 0;
}

// sys_uname(buf) - Get system name
static int64_t sys_uname(uint64_t buf, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  struct {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
  } *uts = (void *)(uintptr_t)buf;
  
  if (!uts) {
    return -1;
  }
  
  // Copy strings
  const char *sysname = "BrightS";
  const char *nodename = "brights";
  const char *release = "0.1.0";
  const char *version = "BrightS 0.1.0";
  const char *machine = "x86_64";
  
  for (int i = 0; i < 64 && sysname[i]; ++i) uts->sysname[i] = sysname[i];
  for (int i = 0; i < 64 && nodename[i]; ++i) uts->nodename[i] = nodename[i];
  for (int i = 0; i < 64 && release[i]; ++i) uts->release[i] = release[i];
  for (int i = 0; i < 64 && version[i]; ++i) uts->version[i] = version[i];
  for (int i = 0; i < 64 && machine[i]; ++i) uts->machine[i] = machine[i];
  
  return 0;
}

// sys_hostname(name, len) - Get/set hostname
static int64_t sys_hostname(uint64_t name, uint64_t len, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  char *dst = (char *)(uintptr_t)name;
  if (!dst || len == 0) {
    return -1;
  }
  const char *hostname = "brights";
  for (uint64_t i = 0; i < len && hostname[i]; ++i) {
    dst[i] = hostname[i];
  }
  return 0;
}

// sys_env_get(name, value, len) - Get environment variable
static int64_t sys_env_get(uint64_t name, uint64_t value, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  const char *varname = (const char *)(uintptr_t)name;
  char *dst = (char *)(uintptr_t)value;
  
  if (!varname || !dst || len == 0) {
    return -1;
  }
  
  // Simplified - return basic env vars
  if (str_compare(varname, "PATH") == 0) {
    const char *path = "/bin:/usr/bin";
    for (uint64_t i = 0; i < len && path[i]; ++i) dst[i] = path[i];
    return 0;
  }
  if (str_compare(varname, "HOME") == 0) {
    const char *home = "/usr/home";
    for (uint64_t i = 0; i < len && home[i]; ++i) dst[i] = home[i];
    return 0;
  }
  if (str_compare(varname, "SHELL") == 0) {
    const char *shell = "/bin/sh";
    for (uint64_t i = 0; i < len && shell[i]; ++i) dst[i] = shell[i];
    return 0;
  }
  if (str_compare(varname, "USER") == 0) {
    const char *user = "guest";
    for (uint64_t i = 0; i < len && user[i]; ++i) dst[i] = user[i];
    return 0;
  }
  
  return -1;
}

// sys_env_set(name, value) - Set environment variable
static int64_t sys_env_set(uint64_t name, uint64_t value, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)name; (void)value; (void)a2; (void)a3; (void)a4; (void)a5;
  // Not implemented
  return 0;
}

// sys_hook_register(watch_lo, watch_hi, flags) - Register a syscall hook
static int64_t sys_hook_register(uint64_t watch_lo, uint64_t watch_hi, uint64_t flags, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_sys_hook_register(watch_lo, watch_hi, flags);
}

// sys_hook_unregister(hook_id) - Unregister a syscall hook
static int64_t sys_hook_unregister(uint64_t hook_id, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_sys_hook_unregister(hook_id);
}

// sys_hook_poll(hook_id, event_buf, max_events) - Poll for hook events
static int64_t sys_hook_poll(uint64_t hook_id, uint64_t event_buf, uint64_t max_events, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_sys_hook_poll(hook_id, event_buf, max_events);
}

// sys_hook_wait(hook_id, event_buf, timeout_ms) - Wait for hook events with timeout
static int64_t sys_hook_wait(uint64_t hook_id, uint64_t event_buf, uint64_t timeout_ms, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_sys_hook_wait(hook_id, event_buf, timeout_ms);
}

// sys_hook_info(hook_id, info_buf) - Get hook statistics
static int64_t sys_hook_info(uint64_t hook_id, uint64_t info_buf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_sys_hook_info(hook_id, info_buf);
}

// sys_broadcast(custom_a, custom_b) - Broadcast a custom event to all hook listeners
static int64_t sys_broadcast(uint64_t custom_a, uint64_t custom_b, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_sys_broadcast(custom_a, custom_b);
}

// sys_chmod(path, mode) - Change file permissions
static int64_t sys_chmod(uint64_t path, uint64_t mode, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_vfs2_chmod((const char *)(uintptr_t)path, (uint32_t)mode);
}

// sys_chown(path, uid, gid) - Change file owner
static int64_t sys_chown(uint64_t path, uint64_t uid, uint64_t gid, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_vfs2_chown((const char *)(uintptr_t)path, (uint32_t)uid, (uint32_t)gid);
}

// sys_symlink(target, linkpath) - Create symbolic link
static int64_t sys_symlink(uint64_t target, uint64_t linkpath, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_vfs2_symlink((const char *)(uintptr_t)target, (const char *)(uintptr_t)linkpath);
}

// sys_readlink(path, buf, bufsize) - Read symbolic link target
static int64_t sys_readlink(uint64_t path, uint64_t buf, uint64_t bufsize, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_vfs2_readlink((const char *)(uintptr_t)path, (char *)(uintptr_t)buf, bufsize);
}

// sys_socket(domain, type, protocol)
static int64_t sys_socket(uint64_t domain, uint64_t type, uint64_t protocol, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_socket((int)domain, (int)type, (int)protocol);
}

// sys_bind(sockfd, addr_ptr, port)
static int64_t sys_bind(uint64_t sockfd, uint64_t addr_ptr, uint64_t port, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_bind((int)sockfd, *(uint32_t *)(uintptr_t)addr_ptr, (uint16_t)port);
}

// sys_listen(sockfd, backlog)
static int64_t sys_listen(uint64_t sockfd, uint64_t backlog, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_listen((int)sockfd, (int)backlog);
}

// sys_accept(sockfd, addr_ptr, port_ptr)
static int64_t sys_accept(uint64_t sockfd, uint64_t addr_ptr, uint64_t port_ptr, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  uint32_t addr = 0;
  uint16_t port = 0;
  int ret = brights_accept((int)sockfd, &addr, &port);
  if (ret >= 0 && addr_ptr) {
    *(uint32_t *)(uintptr_t)addr_ptr = addr;
  }
  if (ret >= 0 && port_ptr) {
    *(uint16_t *)(uintptr_t)port_ptr = port;
  }
  return ret;
}

// sys_connect(sockfd, addr, port)
static int64_t sys_connect(uint64_t sockfd, uint64_t addr, uint64_t port, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_connect((int)sockfd, (uint32_t)addr, (uint16_t)port);
}

// sys_send(sockfd, buf, len)
static int64_t sys_send(uint64_t sockfd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_send((int)sockfd, (const void *)(uintptr_t)buf, (uint32_t)len);
}

// sys_recv(sockfd, buf, len)
static int64_t sys_recv(uint64_t sockfd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_recv((int)sockfd, (void *)(uintptr_t)buf, (uint32_t)len);
}

// sys_close_socket(sockfd)
static int64_t sys_close_socket(uint64_t sockfd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_close((int)sockfd);
}

// sys_setsockopt(sockfd, level, optname)
static int64_t sys_setsockopt(uint64_t sockfd, uint64_t level, uint64_t optname, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)sockfd; (void)level; (void)optname; (void)a3; (void)a4; (void)a5;
  return 0; /* Not implemented */
}

// sys_ifconfig(cmd) - 0=init, 1=print
static int64_t sys_ifconfig(uint64_t cmd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  if (cmd == 0) {
    brights_net_init();
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    brights_netif_add("eth0", mac);
    brights_netif_set_ip("eth0", 0xC0A80164, 0xFFFFFF00, 0xC0A80101);
    brights_netif_up("eth0");
    return 0;
  }
  return -1;
}

// sys_icmp_echo(dst_ip)
static int64_t sys_icmp_echo(uint64_t dst_ip, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_icmp_echo_request((uint32_t)dst_ip);
}

// sys_ip_parse(str)
static int64_t sys_ip_parse(uint64_t str, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_ip_parse((const char *)(uintptr_t)str);
}

// sys_ip_to_str(ip, buf)
static int64_t sys_ip_to_str(uint64_t ip, uint64_t buf, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  brights_ip_to_str((uint32_t)ip, (char *)(uintptr_t)buf);
  return 0;
}

// System call table
// Numbers follow Linux convention where possible
brights_sysent_t brights_sysent_table[] = {
  {0, sys_nosys},              // 0: unused
  {3, sys_read},               // 1: read
  {3, sys_write},              // 2: write
  {2, sys_open},               // 3: open
  {1, sys_close},              // 4: close
  {1, sys_exit},               // 5: exit
  {0, sys_fork},               // 6: fork
  {3, sys_exec},               // 7: exec
  {2, sys_wait},               // 8: wait
  {0, sys_getpid},             // 9: getpid
  {0, sys_getppid},            // 10: getppid
  {1, sys_sleep_ms},           // 11: sleep_ms
  {0, sys_clock_ns},           // 12: clock_ns
  {0, sys_clock_ms},           // 13: clock_ms
  {1, sys_mkdir},              // 14: mkdir
  {1, sys_unlink},             // 15: unlink
  {1, sys_create},             // 16: create
  {3, sys_lseek},              // 17: lseek
  {2, sys_stat},               // 18: stat
  {6, sys_mmap},               // 19: mmap
  {2, sys_munmap},             // 20: munmap
  {1, sys_brk},                // 21: brk
  {1, sys_signal_raise},       // 22: signal_raise
  {0, sys_signal_pending},     // 23: signal_pending
  {1, sys_signal_handler},     // 24: signal_handler
  {0, sys_yield},              // 25: yield
  {1, sys_pipe},               // 26: pipe
  {1, sys_dup},                // 27: dup
  {2, sys_dup2},               // 28: dup2
  {3, sys_ioctl},              // 29: ioctl
  {2, sys_getcwd},             // 30: getcwd
  {1, sys_chdir},              // 31: chdir
  {3, sys_readdir},            // 32: readdir
  {5, sys_mount},              // 33: mount
  {1, sys_umount},             // 34: umount
  {1, sys_reboot},             // 35: reboot
  {0, sys_shutdown},           // 36: shutdown
  {1, sys_sysinfo},            // 37: sysinfo
  {1, sys_uname},              // 38: uname
  {2, sys_hostname},           // 39: hostname
  {3, sys_env_get},            // 40: env_get
  {2, sys_env_set},            // 41: env_set
  {3, sys_hook_register},      // 42: hook_register
  {1, sys_hook_unregister},    // 43: hook_unregister
  {3, sys_hook_poll},          // 44: hook_poll
  {3, sys_hook_wait},          // 45: hook_wait
  {2, sys_hook_info},          // 46: hook_info
  {2, sys_broadcast},          // 47: broadcast
  {2, sys_chmod},              // 48: chmod
  {3, sys_chown},              // 49: chown
  {2, sys_symlink},            // 50: symlink
  {3, sys_readlink},           // 51: readlink
  {3, sys_socket},             // 52: socket
  {3, sys_bind},               // 53: bind
  {2, sys_listen},             // 54: listen
  {3, sys_accept},             // 55: accept
  {3, sys_connect},            // 56: connect
  {3, sys_send},               // 57: send
  {3, sys_recv},               // 58: recv
  {1, sys_close_socket},       // 59: close_socket
  {3, sys_setsockopt},         // 60: setsockopt
  {1, sys_ifconfig},           // 61: ifconfig
  {1, sys_icmp_echo},          // 62: icmp_echo
  {1, sys_ip_parse},           // 63: ip_parse
  {2, sys_ip_to_str},          // 64: ip_to_str
};

const uint64_t brights_sysent_count = sizeof(brights_sysent_table) / sizeof(brights_sysent_table[0]);
