cmake_minimum_required(VERSION 3.20)
project(unix_v6_modern C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UEFI_CLANG "Build UEFI app with clang/lld" ON)

add_subdirectory(sys)
enable_testing()
add_subdirectory(tests)
