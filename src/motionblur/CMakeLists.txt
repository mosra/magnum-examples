find_package(Magnum REQUIRED
    GlutApplication
    MeshTools
    Primitives
    SceneGraph
    Shaders)

include_directories(${MAGNUM_INCLUDE_DIRS})

corrade_add_resource(MotionBlurShaders shaders
    MotionBlurShader.frag
    MotionBlurShader.vert)

add_executable(motionblurExample
    MotionBlurCamera.cpp
    MotionBlurExample.cpp
    Icosphere.cpp
    ${MotionBlurShaders})
target_link_libraries(motionblurExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES}
    ${MAGNUM_SHADERS_LIBRARIES})
