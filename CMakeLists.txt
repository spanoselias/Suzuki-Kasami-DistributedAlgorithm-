cmake_minimum_required(VERSION 3.2)
project(DistributedAlgorithms)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#Makefile for ABD algorithm
set(client_SOURCE_FILES ABD/src/client.c ABD/src/client.h)
set(direc_SOURCE_FILES ABD/src/directory.c ABD/src/directory.h)

#Makefile for file transfer protocol
set(ftp_client_SOURCE_FILES SFTP/sftp_cli/sftp_cli.c SFTP/sftp_cli/sftp_cli.h )
set(ftp_Server_SOURCE_FILES SFTP/sftp_rep/sftp_rep.c)

#Executable for ABD algorithm
add_executable(cli_exec ${client_SOURCE_FILES})
add_executable(direc_exec ${direc_SOURCE_FILES})

#Executable for file transfer protocol
add_executable(ftp_Ser ${ftp_Server_SOURCE_FILES})
add_executable(ftp_cli ${ftp_client_SOURCE_FILES})