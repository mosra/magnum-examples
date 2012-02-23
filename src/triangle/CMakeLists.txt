corrade_add_resource(IdentityShader shader IdentityShader.frag IdentityShader.vert)

add_executable(triangleExample TriangleExample.cpp IdentityShader.cpp Triangle.cpp ${IdentityShader})
target_link_libraries(triangleExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY})
