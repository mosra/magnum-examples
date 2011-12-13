configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(TexturedTriangleData data TexturedIdentityShader.frag TexturedIdentityShader.vert stone.tga)

add_executable(texturedTriangleExample main.cpp TexturedIdentityShader.cpp TexturedTriangle.cpp ${TexturedTriangleData})
target_link_libraries(texturedTriangleExample ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY})
