if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2019" call "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2015" call "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/vcvarsall.bat" x64 || exit /b
set PATH=%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build Bullet
IF NOT EXIST %APPVEYOR_BUILD_FOLDER%\2.86.1.zip appveyor DownloadFile https://github.com/bulletphysics/bullet3/archive/2.86.1.zip || exit /b
7z x 2.86.1.zip || exit /b
cd bullet3-2.86.1 || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/bullet ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DUSE_GRAPHICAL_BENCHMARK=OFF ^
    -DBUILD_CPU_DEMOS=OFF ^
    -DBUILD_BULLET2_DEMOS=OFF ^
    -DBUILD_BULLET3=OFF ^
    -DBUILD_EXTRAS=OFF ^
    -DBUILD_OPENGL3_DEMOS=OFF ^
    -DINSTALL_LIBS=ON ^
    -DBUILD_UNIT_TESTS=OFF ^
    -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON ^
    -G Ninja || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Corrade
git clone --depth 1 git://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_INTERCONNECT=ON ^
    -DWITH_TESTSUITE=OFF ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 git://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH="%APPVEYOR_BUILD_FOLDER%/SDL;%APPVEYOR_BUILD_FOLDER%/openal" ^
    -DWITH_AUDIO=ON ^
    -DWITH_DEBUGTOOLS=ON ^
    -DWITH_MESHTOOLS=ON ^
    -DWITH_PRIMITIVES=ON ^
    -DWITH_SCENEGRAPH=ON ^
    -DWITH_SHADERS=ON ^
    -DWITH_SHADERTOOLS=%ENABLE_VULKAN% ^
    -DWITH_TEXT=ON ^
    -DWITH_TEXTURETOOLS=ON ^
    -DWITH_TRADE=ON ^
    -DWITH_SDL2APPLICATION=ON ^
    -DWITH_WGLCONTEXT=ON ^
    -DWITH_VK=%ENABLE_VULKAN% ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git || exit /b
cd magnum-integration || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/bullet ^
    -DIMGUI_DIR=%APPVEYOR_BUILD_FOLDER%/deps/imgui ^
    -DWITH_BULLET=ON ^
    -DWITH_DART=OFF ^
    -DWITH_IMGUI=ON ^
    -DWITH_OVR=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Extras
git clone --depth 1 git://github.com/mosra/magnum-extras.git || exit /b
cd magnum-extras || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_UI=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%APPVEYOR_BUILD_FOLDER%/deps;%APPVEYOR_BUILD_FOLDER%/SDL;%APPVEYOR_BUILD_FOLDER%/openal;%APPVEYOR_BUILD_FOLDER%/bullet" ^
    -DIMGUI_DIR=%APPVEYOR_BUILD_FOLDER%/deps/imgui ^
    -DWITH_ANIMATED_GIF_EXAMPLE=ON ^
    -DWITH_ARCBALL_EXAMPLE=ON ^
    -DWITH_AREALIGHTS_EXAMPLE=ON ^
    -DWITH_AUDIO_EXAMPLE=ON ^
    -DWITH_BOX2D_EXAMPLE=OFF ^
    -DWITH_BULLET_EXAMPLE=ON ^
    -DWITH_CUBEMAP_EXAMPLE=ON ^
    -DWITH_DART_EXAMPLE=OFF ^
    -DWITH_FLUIDSIMULATION2D_EXAMPLE=ON ^
    -DWITH_FLUIDSIMULATION3D_EXAMPLE=ON ^
    -DWITH_IMGUI_EXAMPLE=ON ^
    -DWITH_MOTIONBLUR_EXAMPLE=ON ^
    -DWITH_MOUSEINTERACTION_EXAMPLE=ON ^
    -DWITH_OCTREE_EXAMPLE=ON ^
    -DWITH_OVR_EXAMPLE=ON ^
    -DWITH_PICKING_EXAMPLE=ON ^
    -DWITH_PRIMITIVES_EXAMPLE=ON ^
    -DWITH_RAYTRACING_EXAMPLE=ON ^
    -DWITH_SHADOWS_EXAMPLE=ON ^
    -DWITH_TEXT_EXAMPLE=ON ^
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON ^
    -DWITH_TRIANGLE_EXAMPLE=ON ^
    -DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=ON ^
    -DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF ^
    -DWITH_TRIANGLE_VULKAN_EXAMPLE=%ENABLE_VULKAN% ^
    -DWITH_VIEWER_EXAMPLE=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
