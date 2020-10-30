#!/bin/bash
set -ev

git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build || exit /b
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
mkdir build-android-arm && cd build-android-arm
cmake .. \
    -DCMAKE_ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r16b \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=22 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/platforms/android-22/arch-arm64/usr \
    -DCORRADE_INCLUDE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/sysroot/usr \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-android-arm && cd build-android-arm
cmake .. \
    -DCMAKE_ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r16b \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=22 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/platforms/android-22/arch-arm64/usr \
    -DMAGNUM_INCLUDE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/sysroot/usr \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_SHADERTOOLS=OFF \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_TRADE=ON \
    -DWITH_SDL2APPLICATION=OFF \
    -DWITH_ANDROIDAPPLICATION=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-android-arm && cd build-android-arm
cmake .. \
    -DCMAKE_ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r16b \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=22 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/platforms/android-22/arch-arm64/usr \
    -DMAGNUM_INCLUDE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/sysroot/usr \
    -DIMGUI_DIR=$HOME/imgui \
    -DWITH_BULLET=OFF \
    -DWITH_DART=OFF \
    -DWITH_IMGUI=ON \
    -DWITH_OVR=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 git://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-android-arm && cd build-android-arm
cmake .. \
    -DCMAKE_ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r16b \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=22 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/platforms/android-22/arch-arm64/usr \
    -DMAGNUM_INCLUDE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/android-ndk-r16b/sysroot/usr \
    -DWITH_UI=OFF \
    -G Ninja
ninja install
cd ../..

export ANDROID_NDK_HOME=$TRAVIS_BUILD_DIR/android-ndk-r16b
export CMAKE_PREFIX_PATH=$HOME/deps-native/
