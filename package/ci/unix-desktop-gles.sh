#!/bin/bash
set -ev

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DMAGNUM_TARGET_GLES=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_TARGET_EGL=OFF \
    -DMAGNUM_WITH_AUDIO=ON \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -G Ninja
ninja install
cd ../..

# Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DMAGNUM_WITH_BULLETINTEGRATION=ON \
    -DMAGNUM_WITH_DARTINTEGRATION=OFF \
    -DMAGNUM_WITH_IMGUIINTEGRATION=ON \
    -DMAGNUM_WITH_OVRINTEGRATION=OFF \
    -G Ninja
ninja install
cd ../..

# Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DMAGNUM_WITH_UI=OFF \
    -G Ninja
ninja install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=ON \
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=OFF \
    -DMAGNUM_WITH_AUDIO_EXAMPLE=ON \
    -DMAGNUM_WITH_BOX2D_EXAMPLE=ON \
    -DMAGNUM_WITH_BULLET_EXAMPLE=ON \
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=OFF \
    -DMAGNUM_WITH_DART_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=OFF \
    -DMAGNUM_WITH_IMGUI_EXAMPLE=ON \
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=OFF \
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=OFF \
    -DMAGNUM_WITH_OCTREE_EXAMPLE=ON \
    -DMAGNUM_WITH_OVR_EXAMPLE=OFF \
    -DMAGNUM_WITH_PICKING_EXAMPLE=OFF \
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON \
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=ON \
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXT_EXAMPLE=ON \
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON \
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DMAGNUM_WITH_VIEWER_EXAMPLE=ON \
    -G Ninja
ninja $NINJA_JOBS

# Test install, after running the tests as for them it shouldn't be needed
ninja install
