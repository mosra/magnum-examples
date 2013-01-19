cmake_minimum_required(VERSION 2.6)

find_package(Magnum REQUIRED
    GlutApplication
    MeshTools
    SceneGraph
    Shaders)

if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${MAGNUM_INCLUDE_DIRS})

corrade_add_resource(PoolShader pool
    PoolShader.vert
    PoolShader.frag
    normal.tga)

add_executable(pool
    Ducks.cpp
    Pool.cpp
    PoolShader.cpp
    Quad.cpp
    ${PoolShader})

target_link_libraries(pool
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES}
    ${MAGNUM_SHADERS_LIBRARIES})
