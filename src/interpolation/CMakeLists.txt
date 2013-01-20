find_package(Magnum REQUIRED GlutApplication DebugTools SceneGraph)

include_directories(${MAGNUM_INCLUDE_DIRS})

add_executable(interpolation Interpolation.cpp)
target_link_libraries(interpolation
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_GLUTAPPLICATION_LIBRARIES}
    ${MAGNUM_DEBUGTOOLS_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES})
