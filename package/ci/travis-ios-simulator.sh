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
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=OFF \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=$TARGET_GLES3 \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_GLFWAPPLICATION=OFF \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -DMAGNUM_WITH_TGAIMPORTER=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
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
    -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=OFF \
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=OFF \
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=OFF \
    -DMAGNUM_WITH_AUDIO_EXAMPLE=OFF \
    -DMAGNUM_WITH_BOX2D_EXAMPLE=OFF \
    -DMAGNUM_WITH_BULLET_EXAMPLE=OFF \
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_DART_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=OFF \
    -DMAGNUM_WITH_IMGUI_EXAMPLE=ON \
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=OFF \
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=OFF \
    -DMAGNUM_WITH_OCTREE_EXAMPLE=OFF \
    -DMAGNUM_WITH_OVR_EXAMPLE=OFF \
    -DMAGNUM_WITH_PICKING_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON \
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=OFF \
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXT_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=ON \
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON \
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DMAGNUM_WITH_VIEWER_EXAMPLE=OFF \
    -G Xcode
set -o pipefail && cmake --build . --config Release | xcpretty
