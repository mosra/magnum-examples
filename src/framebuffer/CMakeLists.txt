if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(ColorCorrectionShader shader ColorCorrectionShader.frag ColorCorrectionShader.vert)

add_executable(framebufferExample FramebufferExample.cpp Billboard.cpp ColorCorrectionCamera.cpp ColorCorrectionShader.cpp ${ColorCorrectionShader})
target_link_libraries(framebufferExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_PRIMITIVES_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY})

