cmake_minimum_required(VERSION 3.15)
project(sin_array
    VERSION 1.0.0
    DESCRIPTION "float/double array with sin"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    else()
        add_compile_options(-O0 -g)
    endif()
    
elseif(MSVC)
    add_compile_options(/W3)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    else()
        add_compile_options(/Od /Zi)
    endif()
endif()

add_executable(sin_array)

target_sources(sin_array
    PRIVATE
        main.cpp
)

target_link_libraries(sin_array
    PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:m>
)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")