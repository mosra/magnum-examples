if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2022" call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
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
git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_WITH_TESTSUITE=OFF ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH="%APPVEYOR_BUILD_FOLDER%/SDL;%APPVEYOR_BUILD_FOLDER%/openal" ^
    -DMAGNUM_WITH_AUDIO=ON ^
    -DMAGNUM_WITH_DEBUGTOOLS=ON ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_MESHTOOLS=ON ^
    -DMAGNUM_WITH_PRIMITIVES=ON ^
    -DMAGNUM_WITH_SCENEGRAPH=ON ^
    -DMAGNUM_WITH_SCENETOOLS=OFF ^
    -DMAGNUM_WITH_SHADERS=ON ^
    -DMAGNUM_WITH_SHADERTOOLS=%ENABLE_VULKAN% ^
    -DMAGNUM_WITH_TEXT=ON ^
    -DMAGNUM_WITH_TEXTURETOOLS=ON ^
    -DMAGNUM_WITH_TRADE=ON ^
    -DMAGNUM_WITH_SDL2APPLICATION=ON ^
    -DMAGNUM_WITH_WGLCONTEXT=ON ^
    -DMAGNUM_WITH_VK=%ENABLE_VULKAN% ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git || exit /b
cd magnum-integration || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/bullet ^
    -DIMGUI_DIR=%APPVEYOR_BUILD_FOLDER%/deps/imgui ^
    -DMAGNUM_WITH_BULLET=ON ^
    -DMAGNUM_WITH_DART=OFF ^
    -DMAGNUM_WITH_IMGUI=ON ^
    -DMAGNUM_WITH_OVR=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git || exit /b
cd magnum-extras || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_WITH_UI=ON ^
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
    -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=ON ^
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=ON ^
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=ON ^
    -DMAGNUM_WITH_AUDIO_EXAMPLE=ON ^
    -DMAGNUM_WITH_BOX2D_EXAMPLE=OFF ^
    -DMAGNUM_WITH_BULLET_EXAMPLE=ON ^
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=ON ^
    -DMAGNUM_WITH_DART_EXAMPLE=OFF ^
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=ON ^
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=ON ^
    -DMAGNUM_WITH_IMGUI_EXAMPLE=ON ^
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=ON ^
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=ON ^
    -DMAGNUM_WITH_OCTREE_EXAMPLE=ON ^
    -DMAGNUM_WITH_OVR_EXAMPLE=ON ^
    -DMAGNUM_WITH_PICKING_EXAMPLE=ON ^
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON ^
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=ON ^
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=ON ^
    -DMAGNUM_WITH_TEXT_EXAMPLE=ON ^
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=ON ^
    -DMAGNUM_WITH_TEXTUREDTRIANGLE_VULKAN_EXAMPLE=%ENABLE_VULKAN% ^
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON ^
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=ON ^
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TRIANGLE_VULKAN_EXAMPLE=%ENABLE_VULKAN% ^
    -DMAGNUM_WITH_VIEWER_EXAMPLE=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
