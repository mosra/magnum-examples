find_package(Magnum REQUIRED GlutContext Primitives)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
    ${MAGNUM_GLUTCONTEXT_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES})
