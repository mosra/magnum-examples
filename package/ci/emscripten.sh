#!/bin/bash
set -ev

git submodule update --init

# Crosscompile Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_TESTSUITE=OFF \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_EMSCRIPTENAPPLICATION=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DIMGUI_DIR=$HOME/imgui \
    -DMAGNUM_WITH_BULLET=OFF \
    -DMAGNUM_WITH_DART=OFF \
    -DMAGNUM_WITH_IMGUI=ON \
    -DMAGNUM_WITH_OVR=OFF \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_UI=OFF \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DIMGUI_DIR=$HOME/imgui \
    -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=OFF \
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=OFF \
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=OFF \
    -DMAGNUM_WITH_AUDIO_EXAMPLE=OFF \
    -DMAGNUM_WITH_BOX2D_EXAMPLE=OFF \
    -DMAGNUM_WITH_BULLET_EXAMPLE=OFF \
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=OFF \
    -DMAGNUM_WITH_DART_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=OFF \
    -DMAGNUM_WITH_IMGUI_EXAMPLE=ON \
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=OFF \
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=OFF \
    -DMAGNUM_WITH_OCTREE_EXAMPLE=OFF \
    -DMAGNUM_WITH_OVR_EXAMPLE=OFF \
    -DMAGNUM_WITH_PICKING_EXAMPLE=OFF \
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=OFF \
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=OFF \
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXT_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DMAGNUM_WITH_VIEWER_EXAMPLE=OFF \
    -DMAGNUM_WITH_WEBXR_EXAMPLE=ON \
    $EXTRA_OPTS \
    -G Ninja
ninja $NINJA_JOBS

# Test install, after running the tests as for them it shouldn't be needed
ninja install
