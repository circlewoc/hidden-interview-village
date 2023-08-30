cmake_minimum_required(VERSION 3.0.0)
project(SRaft)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_BUILD_TYPE Debug)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)


set(executable_name sraft_test)
file(GLOB SOURCE_FILES *.h *.proto *.cc)

add_executable(${executable_name} ${SOURCE_FILES})
target_link_libraries(${executable_name} gRPC::grpc++ protobuf::libprotobuf)
protobuf_generate(TARGET ${executable_name})
