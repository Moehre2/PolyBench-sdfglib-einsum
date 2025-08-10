cmake_minimum_required(VERSION 3.16)
project(PolyBench-sdfglib-einsum
    VERSION 0.1
    DESCRIPTION "Einsum plugin of sdfglib on PolyBench"
    LANGUAGES CXX
)
include(GNUInstallDirs)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(sdfglib CONFIG REQUIRED)
find_package(sdfglibEinsum CONFIG REQUIRED)

set(SOURCE_FILES
    src/benchmarks.cpp
    src/optimize.cpp
)

add_executable(optimize ${SOURCE_FILES})
target_include_directories(optimize PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(optimize PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-private-field -Wno-switch -Wno-deprecated-declarations)
target_link_libraries(optimize PUBLIC sdfglib::sdfglib)
target_link_libraries(optimize PUBLIC sdfglib::sdfglib-einsum)
