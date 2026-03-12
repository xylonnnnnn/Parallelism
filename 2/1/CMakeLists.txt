cmake_minimum_required(VERSION 3.16)
project(task1_matvec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_executable(task1_matvec task1_matvec.c)
target_link_libraries(task1_matvec PRIVATE OpenMP::OpenMP_C m)
