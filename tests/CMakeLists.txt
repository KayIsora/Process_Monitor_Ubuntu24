add_executable(test_io_backoff libpm/test_io_backoff.c)
target_link_libraries(test_io_backoff pm)

add_executable(test_cb libpm/test_cb.c)
target_link_libraries(test_cb pm)

add_executable(test_integ_fakefs libpm/test_integ_fakefs.c)
target_link_libraries(test_integ_fakefs pm)

add_executable(stress_writer vts/stress_writer.c)
target_link_libraries(stress_writer PRIVATE m)
set_target_properties(stress_writer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME fakefs_gen COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/gen_fake_procfs.sh /tmp/fake_pm)
set_tests_properties(fakefs_gen PROPERTIES FIXTURES_SETUP FAKEFS)

add_test(NAME test_io_backoff COMMAND test_io_backoff)
set_tests_properties(test_io_backoff PROPERTIES FIXTURES_REQUIRED FAKEFS ENVIRONMENT "PM_FAKE_ROOT=/tmp/fake_pm")

add_test(NAME test_integ_fakefs COMMAND test_integ_fakefs)
set_tests_properties(test_integ_fakefs PROPERTIES FIXTURES_REQUIRED FAKEFS ENVIRONMENT "PM_FAKE_ROOT=/tmp/fake_pm")

add_test(NAME test_cb COMMAND test_cb) # không cần fakefs
# E2E chạy bằng script (đảm bảo script có quyền chạy hoặc gọi qua bash)
add_test(NAME stress_fifo COMMAND ${CMAKE_SOURCE_DIR}/scripts/e2e_runner.sh)

add_executable(stress_vts_client vts/stress_client.c)
set_target_properties(stress_vts_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
