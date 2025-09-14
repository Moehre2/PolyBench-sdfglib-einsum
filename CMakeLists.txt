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
    src/einsum_pipeline.cpp
    src/loop_consume_assignments.cpp
    src/my_loop_distribute.cpp
    src/optimize.cpp
    src/polybench_node.cpp
    src/timer.cpp
)

add_library(optimize ${SOURCE_FILES})
target_include_directories(optimize PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(optimize PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-private-field -Wno-switch -Wno-deprecated-declarations)
target_link_libraries(optimize PUBLIC sdfglib::sdfglib)
target_link_libraries(optimize PUBLIC sdfglib::sdfglib-einsum)

add_executable(optimize_mkl src/optimize_mkl.cpp)
target_include_directories(optimize_mkl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(optimize_mkl optimize)

add_executable(optimize_mkl3 src/optimize_mkl3.cpp)
target_include_directories(optimize_mkl3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(optimize_mkl3 optimize)