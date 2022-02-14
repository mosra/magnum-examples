#!/bin/bash
set -ev

git clone --depth 1 https://github.com/mosra/corrade.git
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
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
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
    -DWITH_ANDROIDAPPLICATION=ON \
    -DWITH_ANYIMAGEIMPORTER=ON \
    -DWITH_TGAIMPORTER=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -G Ninja
ninja install
cd ../..

# So Gradle can find our stuff, hopefully the Android binaries first
export CMAKE_PREFIX_PATH="$HOME/deps/;$HOME/deps-native/"
