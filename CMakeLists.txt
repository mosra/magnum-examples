cmake_minimum_required(VERSION 2.6)
project(MagnumExamples)

option(BUILD_TESTS "Build unit tests." OFF)
if(BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")

add_subdirectory(src)
