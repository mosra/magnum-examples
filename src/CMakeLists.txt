set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++0x")

find_package(GLUT REQUIRED)

include_directories(${CORRADE_INCLUDE_DIR} ${MAGNUM_INCLUDE_DIR})

add_subdirectory(cubemap)
add_subdirectory(triangle)
add_subdirectory(texturedTriangle)
