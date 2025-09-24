# cli/CMakeLists.txt — chỉ define target cho thư mục cli/, không gọi add_subdirectory ở đây

add_executable(system_monitor
  src/system_monitor.c
)

# Xuất ra binary tên "system-monitor"
set_target_properties(system_monitor PROPERTIES OUTPUT_NAME "system-monitor")

# CLI này chỉ dùng socket libc -> không cần link loclib/pm/vtslog
# Nếu sau này cần, mới thêm target_link_libraries(...) tương ứng.

# Cài đặt vào bin/
install(TARGETS system_monitor RUNTIME DESTINATION bin)
