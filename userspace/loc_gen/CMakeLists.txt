# userspace/loc_gen/CMakeLists.txt
add_library(loclib STATIC
  src/room_mem_reader.c
  src/xml_loader.c
  src/room_store.c
  src/vts_adapter.c
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
add_library(loclib
  src/xml_loader.c
  src/room_store.c
  src/room_mem_reader.c
  src/vts_adapter.c
)
target_include_directories(loclib PUBLIC include ../libpm/include)
target_link_libraries(loclib PUBLIC pm vtslog)  # backoff/cb trong libpm; vtslog nếu cần

add_executable(locgend src/loc_main.c)
target_link_libraries(locgend PRIVATE loclib pm)
install(TARGETS locgend RUNTIME DESTINATION bin)

