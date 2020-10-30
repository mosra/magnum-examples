#!/bin/bash
set -ev

git submodule update --init

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF \
    -DWITH_UTILITY=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC=ON \
    -DTESTSUITE_TARGET_XCTEST=ON \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_TESTSUITE=OFF \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2 \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_AUDIO=ON \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_SHADERTOOLS=OFF \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_TRADE=ON \
    -DWITH_GLFWAPPLICATION=OFF \
    -DWITH_SDL2APPLICATION=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
    -DWITH_BULLET=OFF \
    -DWITH_DART=OFF \
    -DWITH_OVR=OFF \
    -DWITH_IMGUI=ON \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 git://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_UI=OFF \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$TRAVIS_BUILD_DIR/sdl2" \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMGUI_DIR=$HOME/imgui \
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
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=OFF \
    -DWITH_TRIANGLE_EXAMPLE=OFF \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DWITH_VIEWER_EXAMPLE=OFF \
    -G Xcode
set -o pipefail && cmake --build . --config Release | xcpretty
