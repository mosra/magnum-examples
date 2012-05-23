find_package(Magnum REQUIRED GlutContext)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)
corrade_add_resource(TexturedTriangleData data TexturedIdentityShader.frag TexturedIdentityShader.vert stone.tga)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(texturedTriangleExample TexturedTriangleExample.cpp TexturedIdentityShader.cpp TexturedTriangle.cpp ${TexturedTriangleData})
target_link_libraries(texturedTriangleExample ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_GLUTCONTEXT_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY})
