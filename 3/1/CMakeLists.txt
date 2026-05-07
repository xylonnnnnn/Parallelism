cmake_minimum_required(VERSION 3.16)
project(task1_matrix_vector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(matrix_vector_benchmark src/matrix_vector_benchmark.cpp)
target_link_libraries(matrix_vector_benchmark PRIVATE Threads::Threads)

target_compile_options(matrix_vector_benchmark PRIVATE -O2 -Wall -Wextra -Wpedantic)
