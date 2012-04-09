set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++0x")

find_package(GLUT REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CORRADE_INCLUDE_DIR} ${MAGNUM_INCLUDE_DIR})

add_library(MagnumExample STATIC
    AbstractExample.cpp
    FpsCounterExample.cpp
)
if(WIN32)
    target_link_libraries(MagnumExample ${CORRADE_UTILITY_LIBRARY})
endif()

add_subdirectory(cubemap)
add_subdirectory(framebuffer)
add_subdirectory(motionblur)
add_subdirectory(triangle)
add_subdirectory(texturedTriangle)
add_subdirectory(viewer)
