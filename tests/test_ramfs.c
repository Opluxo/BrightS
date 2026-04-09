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

static void test_error_conditions(void)
{
  brights_ramfs_init();

  // Test opening non-existent file
  assert(brights_ramfs_open("/nonexistent.txt") < 0);

  // Test reading from invalid file descriptor
  char buf[10];
  assert(brights_ramfs_read(-1, 0, buf, 10) < 0);
  assert(brights_ramfs_read(999, 0, buf, 10) < 0);

  // Test writing to invalid file descriptor
  assert(brights_ramfs_write(-1, 0, buf, 10) < 0);
  assert(brights_ramfs_write(999, 0, buf, 10) < 0);

  // Test stat on non-existent path
  brights_ramfs_stat_t st;
  assert(brights_ramfs_stat("/nonexistent", &st) < 0);

  // Test unlinking non-existent file
  assert(brights_ramfs_unlink("/nonexistent.txt") < 0);

  // Test removing non-empty directory
  assert(brights_ramfs_mkdir("/testdir") >= 0);
  assert(brights_ramfs_create("/testdir/file.txt") >= 0);
  assert(brights_ramfs_unlink("/testdir") < 0); // Should fail

  // Test creating file with invalid path
  assert(brights_ramfs_create("") < 0);
  assert(brights_ramfs_create(NULL) < 0);
}

static void test_edge_cases(void)
{
  brights_ramfs_init();

  // Test empty file operations
  assert(brights_ramfs_create("/empty.txt") >= 0);
  int fd = brights_ramfs_open("/empty.txt");
  assert(fd >= 0);
  assert(brights_ramfs_file_size(fd) == 0);

  // Test reading from empty file
  char buf[1];
  assert(brights_ramfs_read(fd, 0, buf, 1) == 0);

  // Test writing to empty file then reading
  assert(brights_ramfs_write(fd, 0, "x", 1) == 1);
  assert(brights_ramfs_file_size(fd) == 1);
  assert(brights_ramfs_read(fd, 0, buf, 1) == 1);
  assert(buf[0] == 'x');

  brights_ramfs_close(fd);

  // Test very long path names
  char long_path[BRIGHTS_RAMFS_MAX_NAME + 10];
  memset(long_path, 'a', sizeof(long_path) - 1);
  long_path[sizeof(long_path) - 1] = '\0';
  assert(brights_ramfs_create(long_path) < 0); // Should fail due to length

  // Test reasonable path length
  assert(brights_ramfs_create("/test_reasonable_path_length.txt") >= 0);

  // Clean up
  brights_ramfs_unlink("/test_reasonable_path_length.txt");
  brights_ramfs_unlink("/empty.txt");
}

static void test_directory_operations(void)
{
  brights_ramfs_init();

  // Test directory creation and listing
  assert(brights_ramfs_mkdir("/testdir") >= 0);
  assert(brights_ramfs_create("/testdir/file1.txt") >= 0);
  assert(brights_ramfs_create("/testdir/file2.txt") >= 0);
  assert(brights_ramfs_mkdir("/testdir/subdir") >= 0);

  // Test directory stat
  brights_ramfs_stat_t st;
  assert(brights_ramfs_stat("/testdir", &st) == 0);
  assert(st.is_dir == 1);

  // Test file stat
  assert(brights_ramfs_stat("/testdir/file1.txt", &st) == 0);
  assert(st.is_dir == 0);
  assert(st.size == 0); // Empty file

  // Test directory removal (should fail when not empty)
  assert(brights_ramfs_unlink("/testdir") < 0);

  // Remove files first
  assert(brights_ramfs_unlink("/testdir/file1.txt") == 0);
  assert(brights_ramfs_unlink("/testdir/file2.txt") == 0);
  assert(brights_ramfs_unlink("/testdir/subdir") == 0);

  // Now directory should be removable
  assert(brights_ramfs_unlink("/testdir") == 0);
}

int main(void)
{
  test_nested_paths_and_dirs();
  test_parent_must_exist();
  test_dir_unlink_requires_empty();
  test_path_normalization();
  test_error_conditions();
  test_edge_cases();
  test_directory_operations();
  return 0;
}
