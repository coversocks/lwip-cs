cmake_minimum_required(VERSION 3.10)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release")

project(lwip-cs)

# Example lwIP application
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lwip/)


add_subdirectory(src)
