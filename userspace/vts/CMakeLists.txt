# userspace/vts/CMakeLists.txt — tối giản, thứ tự đúng

set(VTS_INC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1) Thư viện logging (vtslog)
add_library(vtslog STATIC
  src/log_write.c
)
set_property(TARGET vtslog PROPERTY C_STANDARD 11)
target_include_directories(vtslog PRIVATE ${VTS_INC})

# 2) Executable vts (offline daemon)
add_executable(vts
  src/main.c
  src/fifo.c
  src/threads_recv.c
  src/threads_coords.c
  src/threads_cli_bridge.c
  src/vts_config.c
  # KHÔNG lặp lại src/log_write.c ở đây (đã nằm trong vtslog)
)
set_property(TARGET vts PROPERTY C_STANDARD 11)
target_include_directories(vts PRIVATE ${VTS_INC})

# 3) Liên kết thư viện cần thiết
find_library(ATOMIC_LIB atomic)
if (ATOMIC_LIB)
  target_link_libraries(vts PRIVATE vtslog pthread ${ATOMIC_LIB})
  target_link_libraries(vtslog PRIVATE ${ATOMIC_LIB})
else()
  # fallback nếu find_library không ra
  target_link_libraries(vts PRIVATE vtslog pthread atomic)
  target_link_libraries(vtslog PRIVATE atomic)
endif()

# (tùy chọn) xuất binary vào build/bin
set_target_properties(vts PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_sources(vts PRIVATE
  src/net_server.c
  src/req_handler.c
  #src/heartbeat.c # optional
)
target_include_directories(vts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

