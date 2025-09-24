cmake_minimum_required(VERSION 3.16)

add_library(pm STATIC
  src/procfs_io.c
  src/circuit_breaker.c
  src/backoff.c     # file rỗng chỉ để link? -> Không cần: backoff là header-only. Bỏ dòng này nếu không có .c
  src/parser.c
)

target_include_directories(pm PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Nếu backoff header-only: xoá src/backoff.c khỏi add_library
