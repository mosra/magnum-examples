if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(PoolShader pool PoolShader.vert PoolShader.frag normal.tga)

add_executable(pool Ducks.cpp Pool.cpp PoolShader.cpp Quad.cpp ${PoolShader})
target_link_libraries(pool MagnumExample ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_SHADERS_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_gl_LIBRARY})
