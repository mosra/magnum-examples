cmake_minimum_required(VERSION 2.6)
project(MagnumExamples)

find_package(Magnum)
find_package(GLUT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++0x")

add_subdirectory(triangle)
add_subdirectory(texturedTriangle)
