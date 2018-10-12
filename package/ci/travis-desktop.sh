#!/bin/bash
set -ev

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_INTERCONNECT=ON \
    -DWITH_TESTSUITE=OFF \
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_AUDIO=ON \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_TRADE=ON \
    -DWITH_${PLATFORM_GL_API}CONTEXT=ON \
    -DWITH_SDL2APPLICATION=ON \
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -G Ninja
ninja install
cd ../..

# Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_BULLET=ON \
    -DWITH_OVR=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 git://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_UI=ON \
    -G Ninja
ninja install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$HOME/glfw" \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_AREALIGHTS_EXAMPLE=ON \
    -DWITH_AUDIO_EXAMPLE=ON \
    -DWITH_BOX2D_EXAMPLE=$WITH_BOX2D \
    -DWITH_BULLET_EXAMPLE=ON \
    -DWITH_CUBEMAP_EXAMPLE=ON \
    -DWITH_MOTIONBLUR_EXAMPLE=ON \
    -DWITH_MOUSEINTERACTION_EXAMPLE=ON \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=ON \
    -DWITH_PRIMITIVES_EXAMPLE=ON \
    -DWITH_SHADOWS_EXAMPLE=ON \
    -DWITH_TEXT_EXAMPLE=ON \
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=ON \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=ON \
    -DWITH_VIEWER_EXAMPLE=ON \
    -G Ninja
# Otherwise the job gets killed (probably because using too much memory)
ninja -j4
