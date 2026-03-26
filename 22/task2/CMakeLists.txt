cmake_minimum_required(VERSION 3.16)
project(task2_integration_openmp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(task2_integration src/task2_integration.cpp)
target_link_libraries(task2_integration PRIVATE OpenMP::OpenMP_CXX)
