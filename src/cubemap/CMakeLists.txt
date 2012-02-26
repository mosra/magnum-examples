if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(CubeMapData data
    CubeMapShader.vert CubeMapShader.frag
    ReflectionShader.vert ReflectionShader.frag)

add_executable(cubeMapExample
    CubeMapExample.cpp
    CubeMap.cpp
    CubeMapShader.cpp
    Reflector.cpp
    ReflectionShader.cpp
    ${CubeMapData})
target_link_libraries(cubeMapExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${CORRADE_PLUGINMANAGER_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_PRIMITIVES_LIBRARY} ${MAGNUM_MESHTOOLS_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY})
