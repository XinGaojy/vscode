# CMake generated Testfile for 
# Source directory: /workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads
# Build directory: /workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[FindThreads.C-only]=] "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/bin/ctest" "--build-and-test" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/C-only" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/C-only" "--build-generator" "Unix Makefiles" "--build-makeprogram" "/usr/bin/gmake" "--build-project" "FindThreads_C-only" "--build-options" "--test-command" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/bin/ctest" "-V")
set_tests_properties([=[FindThreads.C-only]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CMakeLists.txt;2;add_test;/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CMakeLists.txt;0;")
add_test([=[FindThreads.CXX-only]=] "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/bin/ctest" "--build-and-test" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CXX-only" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CXX-only" "--build-generator" "Unix Makefiles" "--build-makeprogram" "/usr/bin/gmake" "--build-project" "FindThreads_CXX-only" "--build-options" "--test-command" "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/bin/ctest" "-V")
set_tests_properties([=[FindThreads.CXX-only]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CMakeLists.txt;2;add_test;/workspace/clang-quickstart/code/cmake/cmake-3.26.4/Tests/FindThreads/CMakeLists.txt;0;")
