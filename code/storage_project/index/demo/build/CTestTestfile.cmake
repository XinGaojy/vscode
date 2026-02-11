# CMake generated Testfile for 
# Source directory: /workspace/linux/clang-quickstart/vscode/code/storage_project/index/demo
# Build directory: /workspace/linux/clang-quickstart/vscode/code/storage_project/index/demo/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(tsdb_tests "/workspace/linux/clang-quickstart/vscode/code/storage_project/index/demo/build/tsdb_tests")
set_tests_properties(tsdb_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/linux/clang-quickstart/vscode/code/storage_project/index/demo/CMakeLists.txt;73;add_test;/workspace/linux/clang-quickstart/vscode/code/storage_project/index/demo/CMakeLists.txt;0;")
subdirs("orc")
