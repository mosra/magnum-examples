find_package(Magnum REQUIRED
    Physics
    Primitives
    SceneGraph)

if(NOT MAGNUM_TARGET_GLES)
    find_package(Magnum REQUIRED GlutWindowContext)
    set(WINDOWCONTEXT_LIBRARIES ${MAGNUM_GLUTWINDOWCONTEXT_LIBRARIES})
else()
    find_package(Magnum REQUIRED XEglWindowContext)
    set(WINDOWCONTEXT_LIBRARIES ${MAGNUM_XEGLWINDOWCONTEXT_LIBRARIES})
endif()

add_executable(physicsExample
    PhysicsExample.cpp)
target_link_libraries(physicsExample
    ${MAGNUM_LIBRARIES}
    ${CORRADE_UTILITY_LIBRARY}
    ${WINDOWCONTEXT_LIBRARIES}
    ${MAGNUM_PHYSICS_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES})
