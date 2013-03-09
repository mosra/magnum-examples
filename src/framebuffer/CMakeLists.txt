find_package(Magnum REQUIRED
    GlutApplication
    Primitives
    SceneGraph)

include_directories(${MAGNUM_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(ColorCorrectionShader shader
    ColorCorrectionShader.frag
    ColorCorrectionShader.vert)

add_executable(framebufferExample
    FramebufferExample.cpp
    ColorCorrectionCamera.cpp
    ColorCorrectionShader.cpp
    Billboard.cpp
    ${ColorCorrectionShader})
target_link_libraries(framebufferExample
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES})
