find_package(Magnum REQUIRED MeshTools Shaders SceneGraph)

include_directories(${MAGNUM_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

add_executable(magnum-viewer ViewerExample.cpp)
target_link_libraries(magnum-viewer
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_EXAMPLE_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_SHADERS_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES})
