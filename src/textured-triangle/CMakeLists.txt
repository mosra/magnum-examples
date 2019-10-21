#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

cmake_minimum_required(VERSION 3.4)

project(MagnumTexturedTriangleExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED GL Shaders Trade)

if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID OR CORRADE_TARGET_IOS)
    find_package(Magnum REQUIRED TgaImporter)
endif()

if(CORRADE_TARGET_ANDROID)
    find_package(Magnum REQUIRED AndroidApplication)
elseif(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(TexturedTriangle_RESOURCES resources.conf)

if(NOT CORRADE_TARGET_ANDROID)
    add_executable(magnum-textured-triangle WIN32
        TexturedTriangleExample.cpp
        ${TexturedTriangle_RESOURCES})
else()
    add_library(magnum-textured-triangle SHARED
        TexturedTriangleExample.cpp
        ${TexturedTriangle_RESOURCES})
endif()
target_link_libraries(magnum-textured-triangle PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Trade)

# Gradle-free Android build
if(CORRADE_TARGET_ANDROID AND CMAKE_ANDROID_NDK)
    include(${PROJECT_SOURCE_DIR}/../../toolchains/modules/UseAndroid.cmake)
    android_create_apk(magnum-textured-triangle src/main/AndroidManifest.xml)
endif()

# iOS-specifics
if(CORRADE_TARGET_IOS)
    set_target_properties(magnum-textured-triangle PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
endif()

if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID OR CORRADE_TARGET_IOS)
    target_link_libraries(magnum-textured-triangle PRIVATE Magnum::TgaImporter)
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES textured-triangle.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/textured-triangle RENAME index.html)
    install(TARGETS magnum-textured-triangle DESTINATION ${MAGNUM_DEPLOY_PREFIX}/textured-triangle)
    install(FILES
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/textured-triangle)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-textured-triangle.js.mem
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-textured-triangle.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/textured-triangle OPTIONAL)

# Installation for desktop
elseif(NOT CORRADE_TARGET_IOS)
    install(TARGETS magnum-textured-triangle DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-textured-triangle)
