#include "sysent.h"
#include "../fs/ramfs.h"

static int64_t sys_nosys(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
  return -1;
}

static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_ramfs_write((int)fd, 0, (const void *)(uintptr_t)buf, len);
}

static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len, uint64_t a3, uint64_t a4, uint64_t a5)
{
  (void)a3; (void)a4; (void)a5;
  return brights_ramfs_read((int)fd, 0, (void *)(uintptr_t)buf, len);
}

brights_sysent_t brights_sysent_table[] = {
  {0, sys_nosys}, // 0
  {3, sys_read},  // 1
  {3, sys_write}, // 2
};

const uint64_t brights_sysent_count = sizeof(brights_sysent_table) / sizeof(brights_sysent_table[0]);
