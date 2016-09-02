cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(NetCDF-Bench)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)


set (CMAKE_C_FLAGS_DEBUG   "-O0 -g3 -DDEBUG")
set (CMAKE_C_FLAGS_RELEASE "-O3")


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/dev/cmake/modules/")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

add_definitions("-DDATATYPE=int")
add_definitions("-DNC_DATATYPE=NC_INT")

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})


find_package(NetCDF REQUIRED)


add_subdirectory(src)




