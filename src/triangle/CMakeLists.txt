corrade_add_resource(IdentityShader shader IdentityShader.frag IdentityShader.vert)

add_executable(triangleExample main.cpp IdentityShader.cpp Triangle.cpp ${IdentityShader})
target_link_libraries(triangleExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${GLUT_LIBRARY} ${GLEW_LIBRARY})
