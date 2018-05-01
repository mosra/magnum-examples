#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
#           Vladimír Vondruš <mosra@centrum.cz>
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
#   this software, either in source code form or as a compiled binary, for any
#   purpose, commercial or non-commercial, and by any means.
#
#   In jurisdictions that recognize copyright laws, the author or authors of
#   this software dedicate any and all copyright interest in the software to
#   the public domain. We make this dedication for the benefit of the public
#   at large and to the detriment of our heirs and successors. We intend this
#   dedication to be an overt act of relinquishment in perpetuity of all
#   present and future rights to this software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 2.8.12)

# CMake policies. 0025 needs to be before project(), so putting all there.
# Use AppleClang instead of Clang on Apple
if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif()
# Don't treat imported targets with :: as files
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()
# Enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()
# Quoted variables should not be dereferenced
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

project(MagnumPrimitivesExample)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../../modules/")
endif()

find_package(Magnum REQUIRED
    GL
    MeshTools
    Primitives
    Shaders)

if(CORRADE_TARGET_ANDROID)
    find_package(Magnum REQUIRED AndroidApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

if(NOT CORRADE_TARGET_ANDROID)
    add_executable(magnum-primitives PrimitivesExample.cpp)
else()
    add_library(magnum-primitives SHARED PrimitivesExample.cpp)
endif()
target_link_libraries(magnum-primitives PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::Shaders)

# iOS-specific
if(CORRADE_TARGET_IOS)
    set_target_properties(magnum-primitives PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES primitives.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/primitives RENAME index.html)
    install(TARGETS magnum-primitives DESTINATION ${MAGNUM_DEPLOY_PREFIX}/primitives)
    install(FILES
        ${CMAKE_SOURCE_DIR}/src/WebApplication.css
        ${CMAKE_SOURCE_DIR}/src/EmscriptenApplication.js
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/primitives)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-primitives.js.mem
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-primitives.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/primitives OPTIONAL)

# Installation for desktop
elseif(NOT CORRADE_TARGET_IOS)
    install(TARGETS magnum-primitives DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()
