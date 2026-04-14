add_executable(test_ramfs
  test_ramfs.c
  ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
)

target_include_directories(test_ramfs PRIVATE
  ${CMAKE_SOURCE_DIR}/kernel
  ${CMAKE_SOURCE_DIR}/kernel/fs
)

target_compile_definitions(test_ramfs PRIVATE BRIGHTS_TEST_BUILD)

add_executable(test_benchmark
  test_benchmark.c
  ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
)

target_include_directories(test_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}/kernel
  ${CMAKE_SOURCE_DIR}/kernel/fs
)

target_compile_definitions(test_benchmark PRIVATE BRIGHTS_TEST_BUILD)

add_test(NAME test_ramfs COMMAND test_ramfs)
add_test(NAME test_benchmark COMMAND test_benchmark)
