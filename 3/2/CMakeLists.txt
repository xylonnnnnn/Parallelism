cmake_minimum_required(VERSION 3.16)
project(lab_task3_solution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_executable(task3_server src/main.cpp)
target_include_directories(task3_server PRIVATE include)
target_link_libraries(task3_server PRIVATE Threads::Threads)

target_compile_options(task3_server PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -O2>
)

add_executable(verify_results src/verify_results.cpp)
target_compile_options(verify_results PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -O2>
)
