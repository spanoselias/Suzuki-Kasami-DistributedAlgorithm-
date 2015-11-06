cmake_minimum_required(VERSION 3.2)
project(FTP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES  src/FTP_Client/client.c)
add_executable(FTP ${SOURCE_FILES})