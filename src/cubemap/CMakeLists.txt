find_package(Magnum REQUIRED GlutContext MeshTools Primitives SceneGraph)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(CubeMapData data
    CubeMapShader.vert CubeMapShader.frag
    ReflectionShader.vert ReflectionShader.frag
    tarnish.tga)

add_executable(cubeMapExample
    CubeMapExample.cpp
    CubeMap.cpp
    CubeMapShader.cpp
    Reflector.cpp
    ReflectionShader.cpp
    ${CubeMapData})
target_link_libraries(cubeMapExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTCONTEXT_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES})
