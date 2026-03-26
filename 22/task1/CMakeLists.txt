cmake_minimum_required(VERSION 3.16)
project(task1_matvec_openmp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(task1_matvec src/task1_matrix_vector.cpp)
target_link_libraries(task1_matvec PRIVATE OpenMP::OpenMP_CXX)
