#!/bin/bash
set -ev

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_WITH_INTERCONNECT=ON \
    -DCORRADE_WITH_TESTSUITE=OFF \
    -DCORRADE_BUILD_DEPRECATED=$BUILD_DEPRECATED \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DMAGNUM_WITH_AUDIO=ON \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=ON \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_${PLATFORM_GL_API}CONTEXT=ON \
    -DMAGNUM_WITH_VK=ON \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -DMAGNUM_BUILD_DEPRECATED=$BUILD_DEPRECATED \
    -G Ninja
ninja install
cd ../..

# Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DMAGNUM_WITH_BULLET=ON \
    -DMAGNUM_WITH_DART=$WITH_DART \
    -DMAGNUM_WITH_IMGUI=ON \
    -DMAGNUM_WITH_OVR=OFF \
    -G Ninja
ninja install
cd ../..

# Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DMAGNUM_WITH_UI=ON \
    -G Ninja
ninja install
cd ../..

# Disabling the sokol_gfx example as it only works with Magnum compiled with
# TARGET_GL disabled -- otherwise the flextGL headers conflict between the two.
# It's a problem since Application classes started including GL/Context.h to
# inherit from GLContext::Configuration.
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DWITH_ANIMATED_GIF_EXAMPLE=ON \
    -DWITH_ARCBALL_EXAMPLE=ON \
    -DWITH_AREALIGHTS_EXAMPLE=ON \
    -DWITH_AUDIO_EXAMPLE=ON \
    -DWITH_BOX2D_EXAMPLE=$WITH_BOX2D \
    -DWITH_BULLET_EXAMPLE=ON \
    -DWITH_CUBEMAP_EXAMPLE=ON \
    -DWITH_DART_EXAMPLE=$WITH_DART \
    -DWITH_FLUIDSIMULATION2D_EXAMPLE=ON \
    -DWITH_FLUIDSIMULATION3D_EXAMPLE=ON \
    -DWITH_IMGUI_EXAMPLE=ON \
    -DWITH_MOTIONBLUR_EXAMPLE=ON \
    -DWITH_MOUSEINTERACTION_EXAMPLE=ON \
    -DWITH_OCTREE_EXAMPLE=ON \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=ON \
    -DWITH_PRIMITIVES_EXAMPLE=ON \
    -DWITH_RAYTRACING_EXAMPLE=ON \
    -DWITH_SHADOWS_EXAMPLE=ON \
    -DWITH_TEXT_EXAMPLE=ON \
    -DWITH_TEXTUREDQUAD_EXAMPLE=ON \
    -DWITH_TEXTUREDTRIANGLE_VULKAN_EXAMPLE=ON \
    -DWITH_TRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=ON \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DWITH_TRIANGLE_VULKAN_EXAMPLE=ON \
    -DWITH_VIEWER_EXAMPLE=ON \
    -G Ninja
ninja $NINJA_JOBS

# Test install, after running the tests as for them it shouldn't be needed
ninja install
