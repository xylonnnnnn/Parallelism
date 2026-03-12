cmake_minimum_required(VERSION 3.16)
project(task2_integrate C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_executable(task2_integrate ../src/task2_integrate.c)
target_link_libraries(task2_integrate PRIVATE OpenMP::OpenMP_C m)
