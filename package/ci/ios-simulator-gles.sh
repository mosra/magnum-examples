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
    -DBUILD_STATIC=ON \
    -DTESTSUITE_TARGET_XCTEST=ON \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_TESTSUITE=OFF \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcbeautify
cd ../..

# Crosscompile SDL. On 2022-14-02 curl says the certificate is expired, so
# ignore that.
# TODO use a CMake build instead
curl --insecure -O https://www.libsdl.org/release/SDL2-2.0.10.tar.gz
tar -xzvf SDL2-2.0.10.tar.gz
cd SDL2-2.0.10/Xcode-iOS/SDL
set -o pipefail && xcodebuild -sdk iphonesimulator13.4 | xcbeautify
cp build/Release-iphonesimulator/libSDL2.a $HOME/deps/lib
mkdir -p $HOME/deps/include/SDL2
cp -R ../../include/* $HOME/deps/include/SDL2
cd ../../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2 \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SCENETOOLS=OFF \
    -DWITH_SHADERS=ON \
    -DWITH_SHADERTOOLS=OFF \
    -DWITH_TEXT=OFF \
    -DWITH_TEXTURETOOLS=OFF \
    -DWITH_TRADE=ON \
    -DWITH_VK=OFF \
    -DWITH_GLFWAPPLICATION=OFF \
    -DWITH_SDL2APPLICATION=ON \
    -DWITH_TGAIMPORTER=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcbeautify
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
    -DWITH_STBIMAGEIMPORTER=ON \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcbeautify
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcbeautify
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcbeautify
cd ../..

# Crosscompile
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -DWITH_ANIMATED_GIF_EXAMPLE=OFF \
    -DWITH_ARCBALL_EXAMPLE=OFF \
    -DWITH_AREALIGHTS_EXAMPLE=OFF \
    -DWITH_AUDIO_EXAMPLE=OFF \
    -DWITH_BOX2D_EXAMPLE=OFF \
    -DWITH_BULLET_EXAMPLE=OFF \
    -DWITH_CUBEMAP_EXAMPLE=$TARGET_GLES3 \
    -DWITH_DART_EXAMPLE=OFF \
    -DWITH_FLUIDSIMULATION2D_EXAMPLE=OFF \
    -DWITH_FLUIDSIMULATION3D_EXAMPLE=OFF \
    -DWITH_IMGUI_EXAMPLE=OFF \
    -DWITH_MOTIONBLUR_EXAMPLE=OFF \
    -DWITH_MOUSEINTERACTION_EXAMPLE=OFF \
    -DWITH_OCTREE_EXAMPLE=OFF \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=OFF \
    -DWITH_PRIMITIVES_EXAMPLE=ON \
    -DWITH_RAYTRACING_EXAMPLE=OFF \
    -DWITH_SHADOWS_EXAMPLE=OFF \
    -DWITH_TEXT_EXAMPLE=OFF \
    -DWITH_TEXTUREDQUAD_EXAMPLE=ON \
    -DWITH_TRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DWITH_VIEWER_EXAMPLE=OFF \
    -G Xcode
set -o pipefail && cmake --build . --config Release | xcbeautify
