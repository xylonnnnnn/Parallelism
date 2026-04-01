
cmake_minimum_required(VERSION 3.16)
project(task1_matrix_vector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(task1_matrix_vector src/main.cpp)
target_link_libraries(task1_matrix_vector PRIVATE OpenMP::OpenMP_CXX)

if (MSVC)
    target_compile_options(task1_matrix_vector PRIVATE /O2)
else()
    target_compile_options(task1_matrix_vector PRIVATE -O3 -march=native)
endif()
