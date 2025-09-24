# cli/CMakeLists.txt
add_executable(pmcli
  src/system_monitor.c)

target_include_directories(pmcli PRIVATE
  ${CMAKE_SOURCE_DIR}/userspace/loc_gen/include
  ${CMAKE_SOURCE_DIR}/userspace/vts/include
  ${CMAKE_SOURCE_DIR}/userspace/libpm/include
)
target_link_libraries(pmcli PRIVATE loclib pm vtslog)
target_link_libraries(pmcli PRIVATE vtslog)
target_link_libraries(pmcli PRIVATE loclib pm)

# và để chắc ăn include được headers:
target_include_directories(pmcli PRIVATE ${CMAKE_SOURCE_DIR}/userspace/vts/include)

