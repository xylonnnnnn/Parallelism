
cmake_minimum_required(VERSION 3.16)
project(task3_linear_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(task3_linear_system src/main.cpp)
target_link_libraries(task3_linear_system PRIVATE OpenMP::OpenMP_CXX)

if (MSVC)
    target_compile_options(task3_linear_system PRIVATE /O2)
else()
    target_compile_options(task3_linear_system PRIVATE -O3 -march=native)
endif()
