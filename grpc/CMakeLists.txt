cmake_minimum_required(VERSION 3.10)
project(dab_grpc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/proto/dab_grpc_api.proto)
set(PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})

set(PROTO_SRCS
    ${PROTO_OUTPUT_DIR}/dab_grpc_api.pb.cc
    ${PROTO_OUTPUT_DIR}/dab_grpc_api.pb.h)
set(GRPC_SRCS
    ${PROTO_OUTPUT_DIR}/dab_grpc_api.grpc.pb.cc
    ${PROTO_OUTPUT_DIR}/dab_grpc_api.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${GRPC_SRCS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
            --cpp_out=${PROTO_OUTPUT_DIR}
            --grpc_out=${PROTO_OUTPUT_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            ${PROTO_INPUT}
    DEPENDS ${PROTO_INPUT}
    VERBATIM)

add_library(dab_grpc_proto STATIC ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(dab_grpc_proto PUBLIC ${PROTO_OUTPUT_DIR})
target_link_libraries(dab_grpc_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)

add_executable(dab_grpc_server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc_server_main.cpp)

target_include_directories(dab_grpc_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${PROTO_OUTPUT_DIR})

target_link_libraries(dab_grpc_server PRIVATE
    dab_grpc_proto
    ofdm_core
    dab_core
    basic_radio
    gRPC::grpc++
    gRPC::grpc++_reflection)
