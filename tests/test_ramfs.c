#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../sys/kernel/fs/ramfs.h"

static void test_nested_paths_and_dirs(void)
{
  brights_ramfs_init();
  assert(brights_ramfs_mkdir("/usr") >= 0);
  assert(brights_ramfs_mkdir("/usr/home") >= 0);
  assert(brights_ramfs_mkdir("/usr/home/guest") >= 0);
  assert(brights_ramfs_create("/usr/home/guest/note.txt") >= 0);

  int fd = brights_ramfs_open("/usr/home/guest/note.txt");
  assert(fd >= 0);
  assert(brights_ramfs_write(fd, 0, "abc", 3) == 3);
  assert(brights_ramfs_file_size(fd) == 3);

  brights_ramfs_stat_t st;
  assert(brights_ramfs_stat("/usr/home/guest", &st) == 0);
  assert(st.is_dir == 1);
  assert(brights_ramfs_stat("/usr/home/guest/note.txt", &st) == 0);
  assert(st.is_dir == 0);
  assert(st.size == 3);
}

static void test_parent_must_exist(void)
{
  brights_ramfs_init();
  assert(brights_ramfs_create("/missing/file.txt") < 0);
  assert(brights_ramfs_mkdir("/missing/dir") < 0);
}

static void test_dir_unlink_requires_empty(void)
{
  brights_ramfs_init();
  assert(brights_ramfs_mkdir("/tmp") >= 0);
  assert(brights_ramfs_create("/tmp/file.txt") >= 0);
  assert(brights_ramfs_unlink("/tmp") < 0);
  assert(brights_ramfs_unlink("/tmp/file.txt") == 0);
  assert(brights_ramfs_unlink("/tmp") == 0);
}

static void test_path_normalization(void)
{
  brights_ramfs_init();
  assert(brights_ramfs_mkdir("/a") >= 0);
  assert(brights_ramfs_mkdir("/a/b") >= 0);
  assert(brights_ramfs_create("/a/b/c.txt") >= 0);
  assert(brights_ramfs_open("//a/./b/../b/c.txt") >= 0);
}

int main(void)
{
  test_nested_paths_and_dirs();
  test_parent_must_exist();
  test_dir_unlink_requires_empty();
  test_path_normalization();
  return 0;
}
