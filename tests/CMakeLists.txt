add_executable(test_btrfs_btree
  test_btrfs_btree.c
  ${CMAKE_SOURCE_DIR}/sys/kernel/fs/btrfs_btree.c
)

target_include_directories(test_btrfs_btree PRIVATE
  ${CMAKE_SOURCE_DIR}/sys/kernel
  ${CMAKE_SOURCE_DIR}/sys/kernel/fs
)

add_test(NAME test_btrfs_btree COMMAND test_btrfs_btree)

add_executable(test_ramfs
  test_ramfs.c
  ${CMAKE_SOURCE_DIR}/sys/kernel/fs/ramfs.c
)

target_include_directories(test_ramfs PRIVATE
  ${CMAKE_SOURCE_DIR}/sys/kernel
  ${CMAKE_SOURCE_DIR}/sys/kernel/fs
)

add_test(NAME test_ramfs COMMAND test_ramfs)
