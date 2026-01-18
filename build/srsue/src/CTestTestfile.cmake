# CMake generated Testfile for 
# Source directory: /home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/srsue/src
# Build directory: /home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/build/srsue/src
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(srsue_version "srsue" "--version")
set_tests_properties(srsue_version PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/srsue/src/CMakeLists.txt;58;add_test;/home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/srsue/src/CMakeLists.txt;0;")
add_test(srsue_help "srsue" "--help")
set_tests_properties(srsue_help PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/srsue/src/CMakeLists.txt;59;add_test;/home/runner/work/Enhanced-srsRAN-4G/Enhanced-srsRAN-4G/srsue/src/CMakeLists.txt;0;")
subdirs("phy")
subdirs("stack")
subdirs("test")
