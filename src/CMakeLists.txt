set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++0x -fvisibility=hidden")

find_package(Magnum REQUIRED GlutContext)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${MAGNUM_INCLUDE_DIRS})

add_library(MagnumExample STATIC
    FpsCounterExample.cpp
)
set(MAGNUM_EXAMPLE_LIBRARIES MagnumExample ${MAGNUM_GLUTCONTEXT_LIBRARIES})

add_subdirectory(cubemap)
add_subdirectory(framebuffer)
add_subdirectory(motionblur)
add_subdirectory(triangle)
add_subdirectory(textured-triangle)
add_subdirectory(viewer)
