cmake_minimum_required(VERSION 3.0)

project(MyMuduoTest)

# Set the source directory
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)

# Find all source and header files in the source directory
file(GLOB SOURCES ${SOURCE_DIR}/*.cc)
file(GLOB HEADERS ${SOURCE_DIR}/*.h)

# Build the shared library
add_library(mymuoduo SHARED ${SOURCES} ${HEADERS})

# Set properties for the shared library
target_include_directories(mymuoduo PUBLIC ${SOURCE_DIR})
target_compile_features(mymuoduo PUBLIC cxx_std_11)

# Add debug flags for the shared library (and executable)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mymuoduo PRIVATE -g -O0)
endif()

# Build the executable using the shared library
add_executable(test test.cc ${HEADERS})

# Set properties for the executable
target_link_libraries(test PRIVATE mymuoduo)

# Add debug flags for the executable
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test PRIVATE -g -O0)
endif()
