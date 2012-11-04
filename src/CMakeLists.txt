set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wold-style-cast -Winit-self -pedantic -std=c++0x -fvisibility=hidden -fno-rtti -fno-exceptions")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdouble-promotion")
endif()

find_package(Magnum REQUIRED)
if(NOT MAGNUM_TARGET_GLES)
    find_package(Magnum REQUIRED GlutWindowContext)
    set(WINDOWCONTEXT_LIBRARIES ${MAGNUM_GLUTWINDOWCONTEXT_LIBRARIES})
else()
    find_package(Magnum REQUIRED XEglWindowContext)
    set(WINDOWCONTEXT_LIBRARIES ${MAGNUM_XEGLWINDOWCONTEXT_LIBRARIES})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${MAGNUM_INCLUDE_DIRS})

add_library(MagnumExample STATIC
    FpsCounterExample.cpp
)
set(MAGNUM_EXAMPLE_LIBRARIES MagnumExample ${WINDOWCONTEXT_LIBRARIES})

if(NOT MAGNUM_TARGET_GLES)
    add_subdirectory(cubemap)
    add_subdirectory(framebuffer)
    add_subdirectory(motionblur)
    add_subdirectory(triangle)
    add_subdirectory(textured-triangle)
endif()

add_subdirectory(physics)
add_subdirectory(viewer)
