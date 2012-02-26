corrade_add_resource(MotionBlurShaders shaders MotionBlurShader.frag MotionBlurShader.vert)

add_executable(motionblurExample MotionBlurExample.cpp Icosphere.cpp MotionBlurCamera.cpp ${MotionBlurShaders})
target_link_libraries(motionblurExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_PRIMITIVES_LIBRARY} ${MAGNUM_SHADERS_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY})
