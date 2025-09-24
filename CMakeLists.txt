cmake_minimum_required(VERSION 3.20)

project(process-monitor
  VERSION 0.1.0
  LANGUAGES C)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Mặc định dùng Clang nếu có (có thể override qua env)
if(NOT CMAKE_C_COMPILER)
  find_program(CLANG_EXECUTABLE NAMES clang)
  if(CLANG_EXECUTABLE)
    set(CMAKE_C_COMPILER "${CLANG_EXECUTABLE}" CACHE FILEPATH "" FORCE)
  endif()
endif()

# Output về ${repo}/build/bin và build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C tiêu chuẩn và warning cơ bản
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(pm_warnings INTERFACE)
target_compile_options(pm_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Werror=return-type)

# Userspace components
add_subdirectory(userspace/libpm)
add_subdirectory(userspace/vts)      # <-- PHẢI đứng trước để tạo target vtslog
add_subdirectory(userspace/loc_gen)
add_subdirectory(cli)

# Tests (cho ctest ở Pha 0, chỉ chạy lệnh true)
enable_testing()
add_subdirectory(tests)

# Kernel module build: để Pha sau, giữ Makefile/Kconfig riêng
message(STATUS "Kernel driver kept out of CMake for Phase 0 (use Kbuild later).")

# Bo sung them thu muc build cho binarys
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
