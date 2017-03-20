#!/bin/bash
set -ev

git submodule update --init

git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build || exit /b
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DCMAKE_INSTALL_RPATH=$HOME/deps-native/lib \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF
make -j install
cd ..

# Crosscompile Corrade
mkdir build-android-arm && cd build-android-arm
ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r10e cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/Android-ARM.cmake \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DWITH_INTERCONNECT=OFF
make -j install
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-android-arm && cd build-android-arm
ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r10e cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/Android-ARM.cmake \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_SHAPES=ON \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_SDL2APPLICATION=OFF \
    -DWITH_ANDROIDAPPLICATION=ON \
    -DTARGET_GLES2=$TARGET_GLES2
make -j install
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-android-arm && cd build-android-arm
ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r10e cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/Android-ARM.cmake \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_BULLET=OFF \
    -DWITH_OVR=OFF
make -j install
cd ../..

# Crosscompile
mkdir build-android-arm && cd build-android-arm
ANDROID_NDK=$TRAVIS_BUILD_DIR/android-ndk-r10e cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/Android-ARM.cmake \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_AUDIO_EXAMPLE=OFF \
    -DWITH_BULLET_EXAMPLE=OFF \
    -DWITH_CUBEMAP_EXAMPLE=OFF \
    -DWITH_MOTIONBLUR_EXAMPLE=OFF \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=OFF \
    -DWITH_PRIMITIVES_EXAMPLE=OFF \
    -DWITH_SHADOWS_EXAMPLE=OFF \
    -DWITH_TEXT_EXAMPLE=OFF \
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=OFF \
    -DWITH_TRIANGLE_EXAMPLE=OFF \
    -DWITH_VIEWER_EXAMPLE=OFF
# Otherwise the job gets killed (probably because using too much memory)
make -j4
