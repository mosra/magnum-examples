corrade_add_resource(benchmark_DATA data ColorShader.frag ColorShader.vert ColorShaderFlat.frag ColorShaderFlat.vert)

add_executable(benchmark ColorShader.cpp Benchmark.cpp Sierpinski.cpp SierpinskiData.cpp ${benchmark_DATA})

target_link_libraries(benchmark MagnumExample ${MAGNUM_LIBRARY} ${MAGNUM_MESHTOOLS_LIBRARY} ${CORRADE_UTILITY_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY})
