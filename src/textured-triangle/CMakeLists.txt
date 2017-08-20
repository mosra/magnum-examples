#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
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
project(MagnumTexturedTriangleExample)

# CMake policies: enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()
# Don't treat imported targets with :: as files
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()

find_package(Magnum REQUIRED
    Shaders
    Sdl2Application)

if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID OR CORRADE_TARGET_IOS)
    find_package(Magnum REQUIRED TgaImporter)
endif()

if(CORRADE_TARGET_ANDROID)
    find_package(Magnum REQUIRED AndroidApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(TexturedTriangle_RESOURCES resources.conf)

if(NOT CORRADE_TARGET_ANDROID)
    add_executable(magnum-textured-triangle
        TexturedTriangleExample.cpp
        ${TexturedTriangle_RESOURCES})
else()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}")
    add_library(magnum-textured-triangle SHARED
        TexturedTriangleExample.cpp
        ${TexturedTriangle_RESOURCES})
endif()
target_include_directories(magnum-textured-triangle PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(magnum-textured-triangle
    Magnum::Application
    Magnum::Magnum
    Magnum::Shaders)

# iOS-specifics
if(CORRADE_TARGET_IOS)
    set_target_properties(magnum-textured-triangle PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
endif()

if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID OR CORRADE_TARGET_IOS)
    target_link_libraries(magnum-textured-triangle Magnum::TgaImporter)
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES textured-triangle-emscripten.html DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(TARGETS magnum-textured-triangle DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/magnum-textured-triangle.js.mem DESTINATION ${MAGNUM_DEPLOY_PREFIX} OPTIONAL)

# Installation for desktop
elseif(NOT CORRADE_TARGET_IOS)
    install(TARGETS magnum-textured-triangle DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
    install(FILES README.md DESTINATION ${MAGNUM_DATA_INSTALL_DIR}/examples RENAME README-textured-triangle.md)
endif()
