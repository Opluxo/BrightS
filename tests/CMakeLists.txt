add_executable(test_ramfs
  test_ramfs.c
  ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
)

target_include_directories(test_ramfs PRIVATE
  ${CMAKE_SOURCE_DIR}/kernel/core
  ${CMAKE_SOURCE_DIR}/kernel/fs
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/kernel
)

target_compile_definitions(test_ramfs PRIVATE BRIGHTS_TEST_BUILD)

add_executable(test_benchmark
  test_benchmark.c
  ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
)

target_include_directories(test_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}/kernel/core
  ${CMAKE_SOURCE_DIR}/kernel/fs
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/kernel
)

target_compile_definitions(test_benchmark PRIVATE BRIGHTS_TEST_BUILD)

add_executable(test_extended
  test_extended.c
  test_stubs.c
  ${CMAKE_SOURCE_DIR}/kernel/core/simd.c
  ${CMAKE_SOURCE_DIR}/kernel/core/cache.c
  ${CMAKE_SOURCE_DIR}/kernel/core/kmalloc.c
  ${CMAKE_SOURCE_DIR}/kernel/core/clock.c
)

target_include_directories(test_extended PRIVATE
  ${CMAKE_SOURCE_DIR}/kernel/core
  ${CMAKE_SOURCE_DIR}/kernel/fs
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/kernel
  ${CMAKE_SOURCE_DIR}/drivers
)

target_compile_definitions(test_extended PRIVATE BRIGHTS_TEST_BUILD)

add_executable(test_kmalloc test_kmalloc.c)

target_include_directories(test_kmalloc PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/kernel
)

add_executable(test_sched test_sched.c)
target_include_directories(test_sched PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/kernel
)

add_test(NAME test_ramfs COMMAND test_ramfs)
add_test(NAME test_benchmark COMMAND test_benchmark)
add_test(NAME test_extended COMMAND test_extended)
add_test(NAME test_kmalloc COMMAND test_kmalloc)
add_test(NAME test_sched COMMAND test_sched)
