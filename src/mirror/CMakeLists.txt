add_executable(mirrorExample
    Floor.cpp
    MirrorExample.cpp
    Player.cpp
    Solid.cpp
)
target_link_libraries(mirrorExample MagnumExample ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${MAGNUM_MESHTOOLS_LIBRARY} ${MAGNUM_SHADERS_LIBRARY} ${MAGNUM_PRIMITIVES_LIBRARY} ${GLEW_LIBRARY} ${GLUT_LIBRARY})

