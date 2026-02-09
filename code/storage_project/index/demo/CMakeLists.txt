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

option(TSDB_ENABLE_ORC "Enable ORC points format support" OFF)
option(TSDB_USE_BUNDLED_ORC "Use bundled ORC source under third_party/orc" OFF)
set(TSDB_ORC_SOURCE_DIR "" CACHE PATH "Path to Apache ORC source tree")
if(TSDB_ENABLE_ORC)
  set(ORC_SOURCE_DIR "")
  if(TSDB_USE_BUNDLED_ORC)
    set(ORC_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/orc")
  elseif(TSDB_ORC_SOURCE_DIR)
    set(ORC_SOURCE_DIR "${TSDB_ORC_SOURCE_DIR}")
  endif()

  if(ORC_SOURCE_DIR)
    if(NOT EXISTS "${ORC_SOURCE_DIR}/CMakeLists.txt")
      message(FATAL_ERROR "ORC source not found at ${ORC_SOURCE_DIR}")
    endif()
    set(BUILD_JAVA OFF CACHE BOOL "" FORCE)
    set(ANALYZE_JAVA OFF CACHE BOOL "" FORCE)
    set(BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(STOP_BUILD_ON_WARNING OFF CACHE BOOL "" FORCE)
    set(ENABLE_ASAN OFF CACHE BOOL "" FORCE)
    set(ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
    add_subdirectory(${ORC_SOURCE_DIR} ${CMAKE_BINARY_DIR}/orc)
    target_link_libraries(tsdb PRIVATE orc)
  else()
    find_path(ORC_INCLUDE_DIR orc/OrcFile.hh)
    find_library(ORC_LIBRARY NAMES orc)
    if(NOT ORC_INCLUDE_DIR OR NOT ORC_LIBRARY)
      message(FATAL_ERROR "Apache ORC C++ headers/libs not found. Set ORC_INCLUDE_DIR/ORC_LIBRARY or enable bundled ORC.")
    endif()
    target_include_directories(tsdb PRIVATE ${ORC_INCLUDE_DIR})
    target_link_libraries(tsdb PRIVATE ${ORC_LIBRARY})
  endif()
  target_compile_definitions(tsdb PRIVATE TSDB_ENABLE_ORC=1)
endif()

add_executable(tsdb_gen
  tools/generate_data.cpp
)
