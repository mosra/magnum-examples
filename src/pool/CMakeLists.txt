corrade_add_resource(PoolShader pool PoolShader.vert PoolShader.frag)

add_executable(pool Pool.cpp PoolShader.cpp Quad.cpp ${PoolShader})
target_link_libraries(pool MagnumExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY})
