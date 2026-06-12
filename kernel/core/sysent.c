#include "sysent.h"
#include "fs/ramfs.h"
#include "fs/boot_fs.h"
#include "fs/vfs2.h"
#include "pipe.h"
#include "pipe_vfs.h"
#include "../net/net.h"
#include "../../config/network.h"
#include "elf.h"
#include "clock.h"
#include "sleep.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "kmalloc.h"
#include "pmem.h"
#include "syshook.h"
#include "../drivers/serial.h"
#include "../drivers/tty.h"

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

// sys_exec(path, argv, envp) - Execute program (POSIX semantics)
static int64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)argv; (void)envp; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) return -1;

  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) {
    int i = 0;
    while (pathname[i] && i < 255) { resolved[i] = pathname[i]; ++i; }
    resolved[i] = 0;
  }

  int fd = brights_ramfs_open(resolved);
  if (fd < 0) {
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

  static uint8_t elf_buf[65536];
  int64_t n = brights_ramfs_read(fd, 0, elf_buf, sizeof(elf_buf));
  brights_ramfs_close(fd);
  if (n <= 0) return -1;

  elf64_load_info_t load_info;
  if (brights_proc_exec(elf_buf, (uint64_t)n, &load_info) != 0) {
    return -1;
  }

#ifndef __i386__
  brights_proc_exec_continue();
#endif

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

// sys_signal_handler(signo, handler) - Set signal handler (like POSIX signal())
static int64_t sys_signal_handler(uint64_t signo, uint64_t handler, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  if (signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return -1;
  if (signo == SIGKILL || signo == SIGSTOP) return -1;
  sighandler_t old = brights_signal_sethandler(brights_signal_global(), (uint32_t)signo, (sighandler_t)(uintptr_t)handler);
  return (int64_t)(uintptr_t)old;
}

// sys_sigaction(signo, act, oldact) - Examine/change signal action
static int64_t sys_sigaction(uint64_t signo, uint64_t act, uint64_t oldact, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  const brights_sigaction_t *act_ptr = (const brights_sigaction_t *)(uintptr_t)act;
  brights_sigaction_t *oldact_ptr = (brights_sigaction_t *)(uintptr_t)oldact;
  return brights_signal_sigaction((uint32_t)signo, act_ptr, oldact_ptr);
}

// sys_kill(pid, signo) - Send signal to process
static int64_t sys_kill(uint64_t pid, uint64_t signo, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  if (signo >= BRIGHTS_SIGNAL_MAX) return -1;
  
  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) return -1;
  
  uint32_t idx = brights_proc_index((uint32_t)pid);
  if (idx >= 64) return -1;
  
  if (table[idx].state == BRIGHTS_PROC_UNUSED) return -1;
  
  return brights_signal_raise_proc(&table[idx].signal, (uint32_t)signo);
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
      brights_proc_reap((uint32_t)pid);
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

  vfs_file_t *rf = 0, *wf = 0;
  if (brights_pipe_vfs_create(&rf, &wf) != 0) return -1;

  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) {
    brights_vfs2_file_free(rf);
    brights_vfs2_file_free(wf);
    return -1;
  }

  int read_fd = -1, write_fd = -1;
  for (int i = 3; i < BRIGHTS_PROC_MAX_FDS; i += 2) {
    if (!ft[i] && !ft[i + 1]) {
      read_fd = i;
      write_fd = i + 1;
      break;
    }
  }
  if (read_fd < 0) {
    brights_vfs2_file_free(rf);
    brights_vfs2_file_free(wf);
    return -1;
  }

  ft[read_fd] = rf;
  ft[write_fd] = wf;
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
  (void)flags; (void)data; (void)a5;
  const char *src = (const char *)(uintptr_t)source;
  const char *tgt = (const char *)(uintptr_t)target;
  const char *type = (const char *)(uintptr_t)fstype;
  (void)tgt; (void)type;
  return brights_boot_fs_mount_external(src ? src : "manual");
}

// sys_umount(target) - Unmount filesystem
static int64_t sys_umount(uint64_t target, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *path = (const char *)(uintptr_t)target;
  if (!path) return -1;
  return brights_vfs2_umount(path);
}

// sys_reboot(cmd) - Reboot system
static int64_t sys_reboot(uint64_t cmd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)cmd; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  
  /* Try keyboard controller reset first (port 0x64) */
  uint8_t good = 0x02;
  while (good & 0x02)
    good = *(uint8_t *)(uintptr_t)0x64;
  *(uint8_t *)(uintptr_t)0x64 = 0xFE;
  
  /* Fallback: triple fault */
#ifdef __x86_64__
  __asm__ __volatile__("lidt (%%rax)" : : "a"((uintptr_t)&(struct {uint16_t limit; uint64_t base;}){0, 0}));
#else
  __asm__ __volatile__("lidt (%%eax)" : : "a"((uintptr_t)&(struct {uint16_t limit; uint32_t base;}){0, 0}));
#endif
  __asm__ __volatile__("ud2");
  
  return 0;
}

// sys_shutdown() - Shutdown system (ACPI power off)
static int64_t sys_shutdown(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  
  /* ACPI shutdown: write to PM1a_CNT register */
  /* Standard ACPI PM1a_CNT is at 0x404 (from FADT) or 0x600+0x04 */
  /* For QEMU/Bochs: write 0x2000 to 0x604 */
  *(uint16_t *)(uintptr_t)0x604 = 0x2000;
  
  /* Fallback: APM shutdown via port 0xB2 */
  *(uint8_t *)(uintptr_t)0xB2 = 0x0001;
  
  /* Final fallback: halt */
  __asm__ __volatile__("cli; hlt");
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
  const char *release = "0.1.2.6";
  const char *version = "BrightS v0.1.2.6";
#ifdef __i386__
  const char *machine = "i386";
#else
  const char *machine = "x86_64";
#endif
  
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
  (void)a2; (void)a3; (void)a4; (void)a5;
  const char *varname = (const char *)(uintptr_t)name;
  const char *varvalue = (const char *)(uintptr_t)value;
  
  if (!varname || !varvalue) {
    return -1;
  }
  
  brights_proc_info_t *table = brights_proc_table_ptr();
  if (!table) return -1;
  
  uint32_t idx = brights_proc_index(brights_proc_current());
  if (idx >= 64) return -1;
  
  brights_proc_info_t *proc = &table[idx];
  
  /* Check if variable already exists, update it */
  for (int i = 0; i < proc->env_count; i++) {
    int match = 1;
    for (int j = 0; j < BRIGHTS_PROC_ENV_KEY_LEN; j++) {
      if (proc->env_keys[i][j] != varname[j]) { match = 0; break; }
      if (varname[j] == 0) break;
    }
    if (match) {
      /* Update value */
      for (int j = 0; j < BRIGHTS_PROC_ENV_VAL_LEN; j++) {
        proc->env_vals[i][j] = varvalue[j];
        if (varvalue[j] == 0) break;
      }
      return 0;
    }
  }
  
  /* Add new variable */
  if (proc->env_count >= BRIGHTS_PROC_ENV_MAX) return -1;
  
  int ci = proc->env_count;
  for (int j = 0; j < BRIGHTS_PROC_ENV_KEY_LEN; j++) {
    proc->env_keys[ci][j] = varname[j];
    if (varname[j] == 0) break;
  }
  for (int j = 0; j < BRIGHTS_PROC_ENV_VAL_LEN; j++) {
    proc->env_vals[ci][j] = varvalue[j];
    if (varvalue[j] == 0) break;
  }
  proc->env_count++;
  
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

// sys_setsockopt(sockfd, level, optname, optval, optlen)
static int64_t sys_setsockopt(uint64_t sockfd, uint64_t level, uint64_t optname, uint64_t optval, uint64_t optlen, uint64_t a5)
{
  if (sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;
  
  // SOL_SOCKET level options
  if (level == 1) { // SOL_SOCKET
    switch (optname) {
      case 1: // SO_REUSEADDR
      case 4: // SO_KEEPALIVE
      case 8: // SO_LINGER
      case 13: // SO_BROADCAST
      case 32: // SO_SNDBUF
      case 33: // SO_RCVBUF
        return 0; // Acknowledged
      default:
        return -1;
    }
  }
  
  // IP level options (level=2 for IP)
  if (level == 2) {
    switch (optname) {
      case 1: // IP_TOS
      case 2: // IP_TTL
      case 3: // IP_HDRINCL
        return 0;
      default:
        return -1;
    }
  }
  
  return 0;
}

// sys_ifconfig(cmd) - 0=init, 1=print
static int64_t sys_ifconfig(uint64_t cmd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  if (cmd == 0) {
    brights_net_init();
    uint8_t mac[6] = BRIGHTS_DEFAULT_MAC;
    brights_netif_add("eth0", mac);
    brights_netif_set_ip("eth0", BRIGHTS_DEFAULT_IP, BRIGHTS_DEFAULT_NETMASK, BRIGHTS_DEFAULT_GATEWAY);
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

// sys_setsid() - create a new session
static int64_t sys_setsid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;
  uint32_t pid = brights_proc_current();
  return (int64_t)pid;
}

// sys_getuid() - get user ID
SYSCALL0(getuid, { return 0; })

// sys_geteuid() - get effective user ID
SYSCALL0(geteuid, { return 0; })

// sys_getgid() - get group ID
SYSCALL0(getgid, { return 0; })

// sys_getegid() - get effective group ID
SYSCALL0(getegid, { return 0; })

// sys_umask(mask) - set file creation mask
static uint32_t current_umask = 0022;
SYSCALL1(umask, { uint32_t old = current_umask; current_umask = (uint32_t)a0 & 0777; return old; })

// sys_gettimeofday(tv, tz) - get time
SYSCALL2(gettimeofday,
{
  uint64_t ms = brights_clock_ms();
  uint64_t sec = ms / 1000;
  uint64_t usec = (ms % 1000) * 1000;
  if (a0) {
    struct { uint64_t tv_sec; uint64_t tv_usec; } *tv = (void *)(uintptr_t)a0;
    tv->tv_sec = sec;
    tv->tv_usec = usec;
  }
  return 0;
})

// sys_clock_gettime(clk_id, tp) - get clock time
SYSCALL2(clock_gettime,
{
  if (!a1) return -1;
  uint64_t ns = brights_clock_ns();
  uint64_t sec = ns / 1000000000ULL;
  uint64_t nsec = ns % 1000000000ULL;
  struct { uint64_t tv_sec; uint64_t tv_nsec; } *tp = (void *)(uintptr_t)a1;
  tp->tv_sec = sec;
  tp->tv_nsec = nsec;
  return 0;
})

// sys_nanosleep(req, rem) - sleep for nanoseconds
SYSCALL2(nanosleep,
{
  if (!a0) return -1;
  struct { uint64_t tv_sec; uint64_t tv_nsec; } *req = (void *)(uintptr_t)a0;
  uint64_t ms = req->tv_sec * 1000 + req->tv_nsec / 1000000;
  brights_sleep_ms(ms);
  return 0;
})

// sys_time(tloc) - get time in seconds
SYSCALL1(time,
{
  uint64_t sec = brights_clock_ms() / 1000;
  if (a0) *(uint64_t *)(uintptr_t)a0 = sec;
  return (int64_t)sec;
})

// sys_access(path, mode) - check file accessibility
SYSCALL2(access,
{
  const char *pathname = (const char *)(uintptr_t)a0;
  if (!pathname) return -1;
  char resolved[256];
  if (brights_proc_resolve_path(pathname, resolved, sizeof(resolved)) != 0) return -1;
  uint64_t size = 0;
  uint32_t mode = 0;
  if (brights_vfs2_stat(resolved, &size, &mode) != 0) return -1;
  return 0;
})

// sys_fstat(fd, statbuf) - get file status by fd
SYSCALL2(fstat,
{
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)a0);
  if (!f || !f->inode) return -1;
  if (a1) {
    struct { uint64_t size; uint32_t mode; uint32_t pad; } *st = (void *)(uintptr_t)a1;
    st->size = f->inode->size;
    st->mode = f->inode->mode;
  }
  return 0;
})

// sys_rename(oldpath, newpath) - rename a file (copy + unlink)
static int64_t sys_rename(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a2; (void)a3; (void)a4; (void)a5;
  const char *oldp = (const char *)(uintptr_t)a0;
  const char *newp = (const char *)(uintptr_t)a1;
  if (!oldp || !newp) return -1;
  char old_res[256];
  char new_res[256];
  if (brights_proc_resolve_path(oldp, old_res, sizeof(old_res)) != 0) return -1;
  if (brights_proc_resolve_path(newp, new_res, sizeof(new_res)) != 0) return -1;
  uint64_t size = 0;
  uint32_t mode = 0;
  if (brights_vfs2_stat(old_res, &size, &mode) != 0) return -1;
  vfs_file_t *oldf = 0;
  if (brights_vfs2_open(old_res, VFS_O_RDONLY, &oldf) != 0) return -1;
  if (brights_vfs2_create(new_res) != 0) { brights_vfs2_close(oldf); return -1; }
  vfs_file_t *newf = 0;
  if (brights_vfs2_open(new_res, VFS_O_WRONLY, &newf) != 0) { brights_vfs2_close(oldf); return -1; }
  uint8_t rbuf[512];
  uint64_t remain = size;
  while (remain > 0) {
    uint64_t chunk = remain > sizeof(rbuf) ? sizeof(rbuf) : remain;
    int64_t n = brights_vfs2_read(oldf, rbuf, chunk);
    if (n <= 0) break;
    int64_t w = brights_vfs2_write(newf, rbuf, (uint64_t)n);
    if (w <= 0) break;
    remain -= (uint64_t)w;
  }
  brights_vfs2_close(oldf);
  brights_vfs2_close(newf);
  brights_vfs2_unlink(old_res);
  return 0;
}

// sys_readv(fd, iov, iovcnt) - read from file descriptor into gather buffers
SYSCALL3(readv,
{
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)a0);
  if (!f) return -1;
  struct iovec { void *base; uint64_t len; } *iov = (struct iovec *)(uintptr_t)a1;
  int iovcnt = (int)a2;
  int64_t total = 0;
  for (int i = 0; i < iovcnt; ++i) {
    int64_t n = brights_vfs2_read(f, iov[i].base, iov[i].len);
    if (n < 0) return total > 0 ? total : -1;
    total += n;
  }
  return total;
})

// sys_writev(fd, iov, iovcnt) - write to file descriptor from scatter buffers
SYSCALL3(writev,
{
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  vfs_file_t *f = brights_vfs2_fd_get(ft, (int)a0);
  if (!f) return -1;
  struct iovec { const void *base; uint64_t len; } *iov = (struct iovec *)(uintptr_t)a1;
  int iovcnt = (int)a2;
  int64_t total = 0;
  for (int i = 0; i < iovcnt; ++i) {
    int64_t n = brights_vfs2_write(f, iov[i].base, iov[i].len);
    if (n < 0) return total > 0 ? total : -1;
    total += n;
  }
  return total;
})

// sys_fcntl(fd, cmd, arg) - manipulate file descriptor
SYSCALL3(fcntl,
{
  vfs_file_t **ft = brights_proc_fd_table();
  if (!ft) return -1;
  int fd = (int)a0;
  int cmd = (int)a1;
  switch (cmd) {
    case 0: // F_DUPFD
      return sys_dup((uint64_t)fd, 0, 0, 0, 0, 0);
    case 1: // F_GETFD
      return 0;
    case 2: // F_SETFD
      return 0;
    case 3: // F_GETFL
      return 0;
    case 4: // F_SETFL
      return 0;
    default:
      return -1;
  }
})

// sys_getsockname(sockfd, addr_ptr, port_ptr) - get socket name
static int64_t sys_getsockname_impl(uint64_t sockfd, uint64_t addr_ptr, uint64_t port_ptr, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  extern brights_socket_t sockets[];
  extern int brights_netif_count(void);
  extern brights_netif_t netifs[];
  
  if (sockfd >= 32) return -1;
  if (!sockets[sockfd].in_use) return -1;
  
  uint32_t *addr = (uint32_t *)(uintptr_t)addr_ptr;
  uint16_t *port = (uint16_t *)(uintptr_t)port_ptr;
  
  if (addr) {
    /* Get local IP from first active interface */
    if (brights_netif_count() > 0) {
      *addr = netifs[0].ip_addr;
    } else {
      *addr = 0;
    }
  }
  if (port) {
    *port = sockets[sockfd].local_port;
  }
  
  return 0;
}

// sys_getpeername(sockfd, addr_ptr, port_ptr) - get peer name
static int64_t sys_getpeername_impl(uint64_t sockfd, uint64_t addr_ptr, uint64_t port_ptr, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  extern brights_socket_t sockets[];
  
  if (sockfd >= 32) return -1;
  if (!sockets[sockfd].in_use) return -1;
  
  uint32_t *addr = (uint32_t *)(uintptr_t)addr_ptr;
  uint16_t *port = (uint16_t *)(uintptr_t)port_ptr;
  
  if (addr) {
    *addr = sockets[sockfd].remote_ip;
  }
  if (port) {
    *port = sockets[sockfd].remote_port;
  }
  
  return 0;
}

// sys_getsockopt(sockfd, level, optname, optval, optlen) - get socket options
SYSCALL5(getsockopt,
{
  return 0;
})

// sys_link(oldpath, newpath) - create a hard link
SYSCALL2(link,
{
  (void)a0; (void)a1;
  return -1;
})

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
  {2, sys_signal_handler},     // 24: signal_handler
  {3, sys_sigaction},          // 25: sigaction
  {2, sys_kill},               // 26: kill
  {0, sys_yield},              // 27: yield
  {1, sys_pipe},               // 28: pipe
  {1, sys_dup},                // 29: dup
  {2, sys_dup2},               // 30: dup2
  {3, sys_ioctl},              // 31: ioctl
  {2, sys_getcwd},             // 32: getcwd
  {1, sys_chdir},              // 33: chdir
  {3, sys_readdir},            // 34: readdir
  {5, sys_mount},              // 35: mount
  {1, sys_umount},             // 36: umount
  {1, sys_reboot},             // 37: reboot
  {0, sys_shutdown},           // 38: shutdown
  {1, sys_sysinfo},            // 39: sysinfo
  {1, sys_uname},              // 40: uname
  {2, sys_hostname},           // 41: hostname
  {3, sys_env_get},            // 42: env_get
  {2, sys_env_set},            // 43: env_set
  {3, sys_hook_register},      // 44: hook_register
  {1, sys_hook_unregister},    // 45: hook_unregister
  {3, sys_hook_poll},          // 46: hook_poll
  {3, sys_hook_wait},          // 47: hook_wait
  {2, sys_hook_info},          // 48: hook_info
  {2, sys_broadcast},          // 49: broadcast
  {2, sys_chmod},              // 50: chmod
  {3, sys_chown},              // 51: chown
  {2, sys_symlink},            // 52: symlink
  {3, sys_readlink},           // 53: readlink
  {3, sys_socket},             // 54: socket
  {3, sys_bind},               // 55: bind
  {2, sys_listen},             // 56: listen
  {3, sys_accept},             // 57: accept
  {3, sys_connect},            // 58: connect
  {3, sys_send},               // 59: send
  {3, sys_recv},               // 60: recv
  {1, sys_close_socket},       // 61: close_socket
  {3, sys_setsockopt},         // 62: setsockopt
  {1, sys_ifconfig},           // 63: ifconfig
  {1, sys_icmp_echo},          // 64: icmp_echo
  {1, sys_ip_parse},           // 65: ip_parse
  {2, sys_ip_to_str},          // 66: ip_to_str
  {0, sys_setsid},             // 67: setsid
  {0, sys_getuid},             // 68: getuid
  {0, sys_geteuid},            // 69: geteuid
  {0, sys_getgid},             // 70: getgid
  {0, sys_getegid},            // 71: getegid
  {1, sys_umask},              // 72: umask
  {2, sys_gettimeofday},       // 73: gettimeofday
  {2, sys_clock_gettime},      // 74: clock_gettime
  {2, sys_nanosleep},          // 75: nanosleep
  {1, sys_time},               // 76: time
  {2, sys_access},             // 77: access
  {2, sys_fstat},              // 78: fstat
  {2, sys_rename},             // 79: rename
  {3, sys_readv},              // 80: readv
  {3, sys_writev},             // 81: writev
  {3, sys_fcntl},              // 82: fcntl
  {3, sys_getsockname_impl},     // 83: getsockname
  {3, sys_getpeername_impl},    // 84: getpeername
  {5, sys_getsockopt},         // 85: getsockopt
  {2, sys_link},               // 86: link
};

const uint64_t brights_sysent_count = sizeof(brights_sysent_table) / sizeof(brights_sysent_table[0]);
