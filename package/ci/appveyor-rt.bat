if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" set GENERATOR=Visual Studio 15 2017
rem This is what should make the *native* corrade-rc getting found. Corrade
rem should not cross-compile it on this platform, because then it'd get found
rem instead of the native version with no way to distinguish the two, and all
rem hell breaks loose. Thus also not passing CORRADE_RC_EXECUTABLE anywhere
rem below to ensure this doesn't regress.
set PATH=%APPVEYOR_BUILD_FOLDER%\deps-native\bin;%PATH%

rem Build ANGLE. The repo is now just a README redirecting to googlesource.
rem I don't want to bother with this right now, so checking out last usable
rem version from 2017. TODO: fix when I can be bothered
git clone --depth 10 https://github.com/MSOpenTech/angle.git || exit /b
cd angle || exit /b
git checkout c61d0488abd9663e0d4d2450db7345baa2c0dfb6 || exit /b
cd winrt\10\src || exit /b
msbuild angle.sln /p:Configuration=Release || exit /b
cd ..\..\..\.. || exit /b

rem Build SDL
appveyor DownloadFile https://www.libsdl.org/release/SDL2-2.0.4.zip || exit /b
7z x SDL2-2.0.4.zip || exit /b
ren SDL2-2.0.4 SDL || exit /b
cd SDL/VisualC-WinRT/UWP_VS2015 || exit/b
msbuild /p:Configuration=Release || exit /b
cd ..\..\..

git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b

rem Build native corrade-rc
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps-native ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_WITH_PLUGINMANAGER=OFF ^
    -DCORRADE_WITH_TESTSUITE=OFF ^
    -DCORRADE_WITH_UTILITY=OFF ^
    -G Ninja || exit /b
cmake --build . --target install || exit /b
cd .. || exit /b

rem Crosscompile Corrade
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_WITH_TESTSUITE=OFF ^
    -DCORRADE_BUILD_STATIC=ON ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DEGL_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libEGL.lib ^
    -DEGL_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DOPENGLES2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DOPENGLES3_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES3_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DSDL2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/SDL/VisualC-WinRT/UWP_VS2015/X64/Release/SDL-UWP/SDL2.lib ^
    -DSDL2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/SDL/include ^
    -DMAGNUM_WITH_AUDIO=OFF ^
    -DMAGNUM_WITH_DEBUGTOOLS=OFF ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_MESHTOOLS=OFF ^
    -DMAGNUM_WITH_PRIMITIVES=OFF ^
    -DMAGNUM_WITH_SCENEGRAPH=OFF ^
    -DMAGNUM_WITH_SHADERS=OFF ^
    -DMAGNUM_WITH_TEXT=OFF ^
    -DMAGNUM_WITH_TEXTURETOOLS=OFF ^
    -DMAGNUM_WITH_TRADE=OFF ^
    -DMAGNUM_WITH_SDL2APPLICATION=ON ^
    -DMAGNUM_TARGET_GLES2=%TARGET_GLES2% ^
    -DMAGNUM_BUILD_STATIC=ON ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Crosscompile Magnum Integration
git clone --depth 1 https://github.com/mosra/magnum-integration.git || exit /b
cd magnum-integration || exit /b
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DOPENGLES2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DOPENGLES3_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES3_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DMAGNUM_WITH_BULLET=OFF ^
    -DMAGNUM_WITH_DART=OFF ^
    -DMAGNUM_WITH_IMGUI=OFF ^
    -DMAGNUM_WITH_OVR=OFF ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Build Magnum Extras
git clone --depth 1 https://github.com/mosra/magnum-extras.git || exit /b
cd magnum-extras || exit /b
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DOPENGLES2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DOPENGLES3_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES3_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DMAGNUM_WITH_UI=OFF ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Crosscompile
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DOPENGLES2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DOPENGLES3_LIBRARY=%APPVEYOR_BUILD_FOLDER%/angle/winrt/10/src/Release_x64/lib/libGLESv2.lib ^
    -DOPENGLES3_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/angle/include ^
    -DSDL2_LIBRARY=%APPVEYOR_BUILD_FOLDER%/SDL/VisualC-WinRT/UWP_VS2015/X64/Release/SDL-UWP/SDL2.lib ^
    -DSDL2_INCLUDE_DIR=%APPVEYOR_BUILD_FOLDER%/SDL/include ^
    -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=OFF ^
    -DMAGNUM_WITH_ARCBALL_EXAMPLE=OFF ^
    -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=OFF ^
    -DMAGNUM_WITH_AUDIO_EXAMPLE=OFF ^
    -DMAGNUM_WITH_BOX2D_EXAMPLE=OFF ^
    -DMAGNUM_WITH_BULLET_EXAMPLE=OFF ^
    -DMAGNUM_WITH_CUBEMAP_EXAMPLE=OFF ^
    -DMAGNUM_WITH_DART_EXAMPLE=OFF ^
    -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=OFF ^
    -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=OFF ^
    -DMAGNUM_WITH_IMGUI_EXAMPLE=OFF ^
    -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=OFF ^
    -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=OFF ^
    -DMAGNUM_WITH_OVR_EXAMPLE=OFF ^
    -DMAGNUM_WITH_OCTREE_EXAMPLE=OFF ^
    -DMAGNUM_WITH_PICKING_EXAMPLE=OFF ^
    -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=OFF ^
    -DMAGNUM_WITH_RAYTRACING_EXAMPLE=OFF ^
    -DMAGNUM_WITH_SHADOWS_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TEXT_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TRIANGLE_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF ^
    -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF ^
    -DMAGNUM_WITH_VIEWER_EXAMPLE=OFF ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release -- /m /v:m || exit /b
