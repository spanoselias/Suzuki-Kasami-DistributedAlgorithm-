cmake_minimum_required(VERSION 3.2)
project(epl601_ex4)


SET(GCC_COVERAGE_COMPILE_FLAGS "-lpthread")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

set(SOURCE_FILES node.c node.h)


add_executable(node ${SOURCE_FILES})
target_link_libraries(node ${GCC_COVERAGE_COMPILE_FLAGS})