cmake_minimum_required(VERSION 2.6)
project(MagnumExamples)

option(BUILD_TESTS "Build unit tests (requires Qt4)." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${MagnumExamples_SOURCE_DIR}/modules/")

if(BUILD_TESTS)
    find_package(Qt4)

    if(NOT QT4_FOUND)
        message(WARNING "Qt4 is required for building unit tests. No tests will be build.")
        set(BUILD_TESTS OFF)
    else()
        enable_testing()
    endif()
endif()

add_subdirectory(src)
