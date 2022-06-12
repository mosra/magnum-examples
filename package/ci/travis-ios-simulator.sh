#!/bin/bash
set -ev

git submodule update --init

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_PLUGINMANAGER=OFF \
    -DCORRADE_WITH_TESTSUITE=OFF \
    -DCORRADE_WITH_UTILITY=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCORRADE_BUILD_STATIC=ON \
    -DCORRADE_TESTSUITE_TARGET_XCTEST=ON \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_TESTSUITE=OFF \
    -DCORRADE_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_AUDIO=ON \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_GLFWAPPLICATION=OFF \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_BULLET=OFF \
    -DMAGNUM_WITH_DART=OFF \
    -DMAGNUM_WITH_OVR=OFF \
    -DMAGNUM_WITH_IMGUI=ON \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_UI=OFF \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$TRAVIS_BUILD_DIR/sdl2" \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DWITH_ANIMATED_GIF_EXAMPLE=OFF \
    -DWITH_ARCBALL_EXAMPLE=OFF \
    -DWITH_AREALIGHTS_EXAMPLE=OFF \
    -DWITH_AUDIO_EXAMPLE=OFF \
    -DWITH_BOX2D_EXAMPLE=OFF \
    -DWITH_BULLET_EXAMPLE=OFF \
    -DWITH_CUBEMAP_EXAMPLE=OFF \
    -DWITH_DART_EXAMPLE=OFF \
    -DWITH_FLUIDSIMULATION2D_EXAMPLE=OFF \
    -DWITH_FLUIDSIMULATION3D_EXAMPLE=OFF \
    -DWITH_IMGUI_EXAMPLE=ON \
    -DWITH_MOTIONBLUR_EXAMPLE=OFF \
    -DWITH_MOUSEINTERACTION_EXAMPLE=OFF \
    -DWITH_OCTREE_EXAMPLE=OFF \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=OFF \
    -DWITH_PRIMITIVES_EXAMPLE=OFF \
    -DWITH_RAYTRACING_EXAMPLE=OFF \
    -DWITH_SHADOWS_EXAMPLE=OFF \
    -DWITH_TEXT_EXAMPLE=OFF \
    -DWITH_TEXTUREDQUAD_EXAMPLE=OFF \
    -DWITH_TRIANGLE_EXAMPLE=OFF \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DWITH_VIEWER_EXAMPLE=OFF \
    -G Xcode
set -o pipefail && cmake --build . --config Release | xcpretty
