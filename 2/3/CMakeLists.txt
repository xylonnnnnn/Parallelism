cmake_minimum_required(VERSION 3.16)
project(task3_slae CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_executable(task3_slae main.cpp)
target_link_libraries(task3_slae PRIVATE OpenMP::OpenMP_CXX m)
