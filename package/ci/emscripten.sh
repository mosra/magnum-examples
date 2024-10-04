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
    -G Ninja
ninja install
cd ../..

# Crosscompile Box2D
wget https://github.com/erincatto/Box2D/archive/v2.3.1.tar.gz
# Used to be named Box2D-2.3.1 in July 2020. Isn't it amazing when release URLs
# that you'd think stay untouched just become a totally different thing
# altogether?!
tar -xzf v2.3.1.tar.gz && cd box2d-2.3.1
mkdir build-emscripten && cd build-emscripten
cmake ../Box2D \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
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
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
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
    -DMAGNUM_WITH_AUDIO=ON \
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
    -DMAGNUM_WITH_TGAIMPORTER=ON \
    -DMAGNUM_WITH_MAGNUMFONT=ON \
    -DMAGNUM_WITH_WAVAUDIOIMPORTER=ON \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_TINYGLTFIMPORTER=ON \
    -DMAGNUM_WITH_STBTRUETYPEFONT=ON \
    -DMAGNUM_WITH_STBVORBISAUDIOIMPORTER=ON \
    -DMAGNUM_WITH_DDSIMPORTER=$TARGET_GLES3
make -j install
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
    -DMAGNUM_WITH_BULLET=ON \
    -DMAGNUM_WITH_DART=OFF \
    -DMAGNUM_WITH_IMGUI=ON \
    -DMAGNUM_WITH_OVR=OFF \
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
    -DMAGNUM_WITH_UI=$TARGET_GLES3 \
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
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_AUDIO_EXAMPLE=ON \
    -DMAGNUM_WITH_BOX2D_EXAMPLE=ON \
    -DMAGNUM_WITH_BULLET_EXAMPLE=ON \
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=OFF \
    -DMAGNUM_WITH_DART_EXAMPLE=OFF \
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_IMGUI_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=OFF \
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_OCTREE_EXAMPLE=ON \
    -DMAGNUM_WITH_OVR_EXAMPLE=OFF \
    -DMAGNUM_WITH_PICKING_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON \
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=$TARGET_GLES3 \
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=OFF \
    -DMAGNUM_WITH_TEXT_EXAMPLE=ON \
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=ON \
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON \
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF \
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
    -DMAGNUM_WITH_VIEWER_EXAMPLE=ON \
    -DMAGNUM_WITH_WEBXR_EXAMPLE=ON \
    -G Ninja
ninja $NINJA_JOBS

# Test install, after running the tests as for them it shouldn't be needed
ninja install
