cmake_minimum_required(VERSION 3.2)
project(DistributedAlgorithms)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES  FTP SERVER/src/Replica/server.c)

add_executable(FTP ${SOURCE_FILES})