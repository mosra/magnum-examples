corrade_add_resource(TexturedTriangleData data TexturedIdentityShader.frag TexturedIdentityShader.vert stone.tga)

add_executable(texturedTriangleExample main.cpp TexturedIdentityShader.cpp TexturedTriangle.cpp TGATexture.cpp ${TexturedTriangleData})
target_link_libraries(texturedTriangleExample Magnum ${GLUT_LIBRARY})
