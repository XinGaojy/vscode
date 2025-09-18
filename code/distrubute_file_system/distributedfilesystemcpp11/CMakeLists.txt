cmake_minimum_required(VERSION 3.10)
project(dfs_cpp11 CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# 1. 优先用 CONFIG 模式（CMake 自带）
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    # 2. 回退到 FindProtobuf.cmake（旧模块模式）
    find_package(Protobuf MODULE REQUIRED)
endif()

# 3. 统一创建别名，保证后面一定能链接到 protobuf::protobuf
if(NOT TARGET protobuf::protobuf AND TARGET protobuf::libprotobuf)
    add_library(protobuf::protobuf ALIAS protobuf::libprotobuf)
elseif(NOT TARGET protobuf::protobuf AND DEFINED PROTOBUF_LIBRARIES)
    # 最旧场景：只有变量
    add_library(protobuf::protobuf INTERFACE IMPORTED)
    set_target_properties(protobuf::protobuf PROPERTIES
        INTERFACE_LINK_LIBRARIES "${PROTOBUF_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIRS}")
endif()

# 4. 同样处理 gRPC
find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    find_package(gRPC MODULE REQUIRED)
endif()
if(NOT TARGET gRPC::grpc++ AND TARGET gRPC::grpcpp)
    add_library(gRPC::grpc++ ALIAS gRPC::grpcpp)
endif()

# ---------------- 后续内容保持原样 ----------------
# proto 生成
set(PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/proto/file.proto")
set(PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})
set(PROTO_SRCS "${PROTO_OUT_DIR}/file.pb.cc")
set(GRPC_SRCS "${PROTO_OUT_DIR}/file.grpc.pb.cc")

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${GRPC_SRCS}
  COMMAND protobuf::protoc
  ARGS --cpp_out ${PROTO_OUT_DIR}
       --grpc_out ${PROTO_OUT_DIR}
       --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
       -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
       ${PROTO_SRC}
  DEPENDS ${PROTO_SRC})

add_library(file_proto STATIC ${PROTO_SRCS} ${GRPC_SRCS})
target_link_libraries(file_proto PUBLIC protobuf::protobuf gRPC::grpc++)

# dfs_lib / server / client 原样不变
add_library(dfs_lib STATIC
  src/common/logger.cc
  src/network/tcp_server.cc
  src/network/tcp_client.cc
  src/protocol/codec.cc
  src/security/crypto.cc
  src/fs/filestore.cc)
target_include_directories(dfs_lib PUBLIC src/)
target_link_libraries(dfs_lib PUBLIC
  file_proto
  protobuf::protobuf
  gRPC::grpc++
  ssl crypto lz4 Threads::Threads)

add_executable(dfs-server src/main_server.cc)
target_link_libraries(dfs-server PRIVATE dfs_lib)

add_executable(dfs-client src/main_client.cc)
target_link_libraries(dfs-client PRIVATE dfs_lib)
