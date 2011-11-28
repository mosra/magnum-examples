corrade_add_resource(IdentityShader shader IdentityShader.frag IdentityShader.vert)

add_executable(triangleExample main.cpp IdentityShader.cpp Triangle.cpp ${IdentityShader})
target_link_libraries(triangleExample Magnum ${GLUT_LIBRARY})
