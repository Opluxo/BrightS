#include "sysent.h"
#include "../fs/ramfs.h"
#include "clock.h"
#include "sleep.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "../dev/serial.h"

static int64_t sys_nosys(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return -1;
}

// sys_read(fd, buf, len) - Read from file descriptor
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_ramfs_read((int)fd, 0, (void *)(uintptr_t)buf, len);
}

// sys_write(fd, buf, len) - Write to file descriptor
static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_ramfs_write((int)fd, 0, (const void *)(uintptr_t)buf, len);
}

// sys_open(path, flags) - Open file
static int64_t sys_open(uint64_t path, uint64_t flags, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)flags; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) {
    return -1;
  }
  return brights_ramfs_open(pathname);
}

// sys_close(fd) - Close file descriptor
static int64_t sys_close(uint64_t fd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  // RAMFS doesn't really close, but we can validate the fd
  if ((int)fd < 0) {
    return -1;
  }
  return 0;
}

// sys_exit(status) - Exit process
static int64_t sys_exit(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)status; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  // For now, just halt
  for (;;) {
    __asm__ __volatile__("hlt");
  }
  return 0; // Never reached
}

// sys_getpid() - Get process ID
static int64_t sys_getpid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return (int64_t)brights_sched_current_pid();
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
  if (!pathname) {
    return -1;
  }
  return brights_ramfs_mkdir(pathname);
}

// sys_unlink(path) - Remove file
static int64_t sys_unlink(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) {
    return -1;
  }
  return brights_ramfs_unlink(pathname);
}

// sys_create(path) - Create file
static int64_t sys_create(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  const char *pathname = (const char *)(uintptr_t)path;
  if (!pathname) {
    return -1;
  }
  return brights_ramfs_create(pathname);
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

// sys_yield() - Yield CPU to next process
static int64_t sys_yield(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return brights_sched_yield();
}

// System call table
// Numbers follow Linux convention where possible
brights_sysent_t brights_sysent_table[] = {
  {0, sys_nosys},        // 0: unused
  {3, sys_read},         // 1: read
  {3, sys_write},        // 2: write
  {2, sys_open},         // 3: open
  {1, sys_close},        // 4: close
  {1, sys_exit},         // 5: exit
  {0, sys_getpid},       // 6: getpid
  {1, sys_sleep_ms},     // 7: sleep_ms
  {0, sys_clock_ns},     // 8: clock_ns
  {0, sys_clock_ms},     // 9: clock_ms
  {1, sys_mkdir},        // 10: mkdir
  {1, sys_unlink},       // 11: unlink
  {1, sys_create},       // 12: create
  {1, sys_signal_raise}, // 13: signal_raise
  {0, sys_signal_pending}, // 14: signal_pending
  {0, sys_yield},        // 15: yield
};

const uint64_t brights_sysent_count = sizeof(brights_sysent_table) / sizeof(brights_sysent_table[0]);
