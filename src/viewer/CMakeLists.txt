if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(magnum-viewer main.cpp)
target_link_libraries(magnum-viewer ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_MESHTOOLS_LIBRARY} ${MAGNUM_SHADERS_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY})
