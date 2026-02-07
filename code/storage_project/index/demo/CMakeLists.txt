cmake_minimum_required(VERSION 3.16)
project(tsdb_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(tsdb
  src/main.cpp
  src/tsdb.cpp
)

target_include_directories(tsdb PRIVATE include)
target_link_libraries(tsdb PRIVATE Threads::Threads)

add_executable(tsdb_gen
  tools/generate_data.cpp
)
