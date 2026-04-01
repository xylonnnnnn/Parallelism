
cmake_minimum_required(VERSION 3.16)
project(task2_integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(task2_integration src/main.cpp)
target_link_libraries(task2_integration PRIVATE OpenMP::OpenMP_CXX)

if (MSVC)
    target_compile_options(task2_integration PRIVATE /O2)
else()
    target_compile_options(task2_integration PRIVATE -O3 -march=native)
endif()
