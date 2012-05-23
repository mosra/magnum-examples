find_package(Magnum REQUIRED GlutContext)

add_executable(triangleExample TriangleExample.cpp TriangleShader.cpp Triangle.cpp)
target_link_libraries(triangleExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_GLUTCONTEXT_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY})
