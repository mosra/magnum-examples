rem Workaround for CMake not wanting sh.exe on PATH for MinGW. AARGH.
set PATH=%PATH:C:\Program Files\Git\usr\bin;=%
set PATH=C:\tools\mingw64\bin;%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build Corrade. Could not get Ninja to work, meh.
git clone --depth 1 git://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_INTERCONNECT=OFF ^
    -G "MinGW Makefiles" || exit /b
cmake --build . -- -j || exit /b
cmake --build . --target install -- -j || exit /b
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
    -DWITH_SHAPES=ON ^
    -DWITH_TEXT=ON ^
    -DWITH_TEXTURETOOLS=ON ^
    -DWITH_SDL2APPLICATION=ON ^
    -G "MinGW Makefiles" || exit /b
cmake --build . -- -j || exit /b
cmake --build . --target install -- -j || exit /b
cd .. && cd ..

rem Build Magnum Integration
git clone --depth 1 git://github.com/mosra/magnum-integration.git || exit /b
cd magnum-integration || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_BULLET=OFF ^
    -DWITH_OVR=ON ^
    -G "MinGW Makefiles" || exit /b
cmake --build . -- -j || exit /b
cmake --build . --target install -- -j || exit /b
cd .. && cd ..

rem Build
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%APPVEYOR_BUILD_FOLDER%/deps;%APPVEYOR_BUILD_FOLDER%/SDL;%APPVEYOR_BUILD_FOLDER%/openal" ^
    -DWITH_AUDIO_EXAMPLE=ON ^
    -DWITH_BULLET_EXAMPLE=OFF ^
    -DWITH_CUBEMAP_EXAMPLE=ON ^
    -DWITH_MOTIONBLUR_EXAMPLE=ON ^
    -DWITH_OVR_EXAMPLE=ON ^
    -DWITH_PICKING_EXAMPLE=ON ^
    -DWITH_PRIMITIVES_EXAMPLE=ON ^
    -DWITH_TEXT_EXAMPLE=ON ^
    -DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON ^
    -DWITH_TRIANGLE_EXAMPLE=ON ^
    -DWITH_VIEWER_EXAMPLE=ON ^
    -G "MinGW Makefiles" || exit /b
cmake --build . -- -j || exit /b
