# CMake generated Testfile for 
# Source directory: /home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests
# Build directory: /home/kayu24/Desktop/Process_Monitor_Ubuntu24/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[fakefs_gen]=] "bash" "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/scripts/gen_fake_procfs.sh" "/tmp/fake_pm")
set_tests_properties([=[fakefs_gen]=] PROPERTIES  FIXTURES_SETUP "FAKEFS" _BACKTRACE_TRIPLES "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;15;add_test;/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;0;")
add_test([=[test_io_backoff]=] "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/build/bin/test_io_backoff")
set_tests_properties([=[test_io_backoff]=] PROPERTIES  ENVIRONMENT "PM_FAKE_ROOT=/tmp/fake_pm" FIXTURES_REQUIRED "FAKEFS" _BACKTRACE_TRIPLES "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;18;add_test;/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;0;")
add_test([=[test_integ_fakefs]=] "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/build/bin/test_integ_fakefs")
set_tests_properties([=[test_integ_fakefs]=] PROPERTIES  ENVIRONMENT "PM_FAKE_ROOT=/tmp/fake_pm" FIXTURES_REQUIRED "FAKEFS" _BACKTRACE_TRIPLES "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;21;add_test;/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;0;")
add_test([=[test_cb]=] "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/build/bin/test_cb")
set_tests_properties([=[test_cb]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;24;add_test;/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;0;")
add_test([=[stress_fifo]=] "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/scripts/e2e_runner.sh")
set_tests_properties([=[stress_fifo]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;26;add_test;/home/kayu24/Desktop/Process_Monitor_Ubuntu24/tests/CMakeLists.txt;0;")
