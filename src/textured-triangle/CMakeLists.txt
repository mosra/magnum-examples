find_package(Magnum REQUIRED GlutApplication)

include_directories(${MAGNUM_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(TexturedTriangleData data
    TexturedTriangleShader.frag
    TexturedTriangleShader.vert
    stone.tga)
add_executable(texturedTriangleExample
    TexturedTriangleExample.cpp
    TexturedTriangleShader.cpp
    ${TexturedTriangleData})
target_link_libraries(texturedTriangleExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES})
