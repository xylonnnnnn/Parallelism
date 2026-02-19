cmake_minimum_required(VERSION 3.16)

project(MatrixMultMT LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable high optimization for Release
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

add_executable(matrix_mult_mt matrix_mult_mt.cpp)

# Threads (portable way)
find_package(Threads REQUIRED)
target_link_libraries(matrix_mult_mt PRIVATE Threads::Threads)

# Optional: show some info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
