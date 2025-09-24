# userspace/loc_gen/CMakeLists.txt
add_library(loclib STATIC
  src/room_mem_reader.c
  src/xml_loader.c
  src/room_store.c
  src/vts_adapter.c
  src/compat_shim.c
)

add_executable(loc_gen
  src/loc_main.c
)

target_include_directories(loclib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/userspace/libpm/include
  ${CMAKE_SOURCE_DIR}/userspace/vts/include
)
target_link_libraries(loc_gen PRIVATE pm vtslog)

#####
add_executable(locgend src/loc_main.c)
target_include_directories(locgend PRIVATE include ../libpm/include)
target_link_libraries(locgend PRIVATE loclib pm)
install(TARGETS locgend RUNTIME DESTINATION bin)
target_include_directories(loc_gen PRIVATE include ../libpm/include)
target_link_libraries(loc_gen PRIVATE loclib pm)

target_compile_definitions(loclib  PRIVATE VTS_PORT=9000)
target_compile_definitions(loc_gen PRIVATE VTS_PORT=9000)
target_compile_definitions(locgend PRIVATE VTS_PORT=9000)