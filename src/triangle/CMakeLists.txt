find_package(Magnum REQUIRED)

corrade_add_resource(IdentityShader shader IdentityShader.frag IdentityShader.vert)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(triangleExample TriangleExample.cpp IdentityShader.cpp Triangle.cpp ${IdentityShader})
target_link_libraries(triangleExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY})
