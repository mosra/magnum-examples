cmake_minimum_required(VERSION 2.6)

find_package(Magnum REQUIRED
    GlutApplication)

include_directories(${MAGNUM_INCLUDE_DIRS})

add_executable(triangleExample
    TriangleExample.cpp
    TriangleShader.cpp)
target_link_libraries(triangleExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES})
