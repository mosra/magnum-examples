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
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum

# The fastest Vulkan driver ever. See travis.yml for why we have a separate
# Vulkan build.
g++ package/ci/libvulkan.cpp -std=c++11 -shared -o $HOME/libvulkan.so

mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DVulkan_LIBRARY=$HOME/libvulkan.so \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_GL=OFF \
    -DWITH_MESHTOOLS=OFF \
    -DWITH_PRIMITIVES=OFF \
    -DWITH_SCENEGRAPH=OFF \
    -DWITH_SHADERS=OFF \
    -DWITH_SHADERTOOLS=ON \
    -DWITH_TEXT=OFF \
    -DWITH_TEXTURETOOLS=OFF \
    -DWITH_TRADE=ON \
    -DWITH_VK=ON \
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -G Ninja
ninja install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DVulkan_LIBRARY=$HOME/libvulkan.so \
    -DWITH_TRIANGLE_EXAMPLE=OFF \
    -DWITH_TRIANGLE_VULKAN_EXAMPLE=ON \
    -G Ninja
# Otherwise the job gets killed (probably because using too much memory)
ninja -j4
