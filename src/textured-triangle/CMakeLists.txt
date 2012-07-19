find_package(Magnum REQUIRED GlutContext)

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
    TexturedTriangle.cpp
    ${TexturedTriangleData})
target_link_libraries(texturedTriangleExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTCONTEXT_LIBRARIES})
