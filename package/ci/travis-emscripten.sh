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
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF \
    -DWITH_UTILITY=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DWITH_INTERCONNECT=$TARGET_GLES3 \
    -DWITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Box2D
wget https://github.com/erincatto/Box2D/archive/v2.3.1.tar.gz
tar -xzf v2.3.1.tar.gz && cd Box2D-2.3.1
mkdir build-emscripten && cd build-emscripten
cmake ../Box2D \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DBOX2D_INSTALL=ON \
    -DBOX2D_INSTALL_DOC=OFF \
    -DBOX2D_BUILD_SHARED=OFF \
    -DBOX2D_BUILD_STATIC=ON \
    -DBOX2D_BUILD_EXAMPLES=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Bullet
wget https://github.com/bulletphysics/bullet3/archive/2.87.tar.gz
tar -xzf 2.87.tar.gz && cd bullet3-2.87
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DBUILD_BULLET2_DEMOS=OFF \
    -DBUILD_BULLET3=OFF \
    -DBUILD_CLSOCKET=OFF \
    -DBUILD_CPU_DEMOS=OFF \
    -DBUILD_ENET=OFF \
    -DBUILD_EXTRAS=OFF \
    -DBUILD_OPENGL3_DEMOS=OFF \
    -DBUILD_PYBULLET=OFF \
    -DBUILD_UNIT_TESTS=OFF \
    -DINSTALL_LIBS=ON \
    -DINSTALL_CMAKE_FILES=OFF \
    -DUSE_GLUT=OFF \
    -DUSE_GRAPHICAL_BENCHMARK=OFF \
    -D_FIND_LIB_PYTHON_PY=$TRAVIS_BUILD_DIR/bullet3-2.87/build3/cmake/FindLibPython.py \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_AUDIO=ON \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_TRADE=ON \
    -DWITH_EMSCRIPTENAPPLICATION=ON \
    -DWITH_TGAIMPORTER=ON \
    -DWITH_MAGNUMFONT=ON \
    -DWITH_WAVAUDIOIMPORTER=ON \
    -DWITH_ANYIMAGEIMPORTER=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 git://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_TINYGLTFIMPORTER=ON \
    -DWITH_STBTRUETYPEFONT=ON \
    -DWITH_STBVORBISAUDIOIMPORTER=ON \
    -DWITH_DDSIMPORTER=$TARGET_GLES3
make -j install
cd ../..

# Crosscompile Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DIMGUI_DIR=$HOME/imgui \
    -DWITH_BULLET=ON \
    -DWITH_DART=OFF \
    -DWITH_IMGUI=$TARGET_GLES3 \
    -DWITH_OVR=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Extras
git clone --depth 1 git://github.com/mosra/magnum-extras.git
cd magnum-extras
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_UI=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Crosscompile
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DIMGUI_DIR=$HOME/imgui \
    -DWITH_AREALIGHTS_EXAMPLE=$TARGET_GLES3 \
    -DWITH_AUDIO_EXAMPLE=ON \
    -DWITH_BOX2D_EXAMPLE=ON \
    -DWITH_BULLET_EXAMPLE=ON \
    -DWITH_CUBEMAP_EXAMPLE=OFF \
    -DWITH_DART_EXAMPLE=OFF \
    -DWITH_FLUIDSIMULATION2D_EXAMPLE=ON \
    -DWITH_FLUIDSIMULATION3D_EXAMPLE=ON \
    -DWITH_IMGUI_EXAMPLE=$TARGET_GLES3 \
    -DWITH_MOTIONBLUR_EXAMPLE=OFF \
    -DWITH_MOUSEINTERACTION_EXAMPLE=$TARGET_GLES3 \
    -DWITH_OVR_EXAMPLE=OFF \
    -DWITH_PICKING_EXAMPLE=$TARGET_GLES3 \
    -DWITH_PRIMITIVES_EXAMPLE=ON \
    -DWITH_SHADOWS_EXAMPLE=OFF \
    -DWITH_TEXT_EXAMPLE=ON \
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_EXAMPLE=ON \
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DWITH_VIEWER_EXAMPLE=ON \
    -DWITH_WEBVR_EXAMPLE=ON \
    -DWITH_WEBXR_EXAMPLE=ON \
    -G Ninja
# Otherwise the job gets killed (probably because using too much memory)
ninja -j4
