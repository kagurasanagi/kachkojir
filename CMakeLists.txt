cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(kachkojir C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_subdirectory(firmware/sd_bridge)