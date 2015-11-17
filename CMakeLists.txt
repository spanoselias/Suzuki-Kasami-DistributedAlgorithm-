cmake_minimum_required(VERSION 3.2)
project(DistributedAlgorithms)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(client_SOURCE_FILES ABD/src/client.c ABD/src/client.h)
set(direc_SOURCE_FILES ABD/src/directory.c ABD/src/directory.h)


add_executable(cli_exec ${direc_SOURCE_FILES})
add_executable(direc_exec ${direc_SOURCE_FILES})
