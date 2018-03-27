#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
#           Vladimír Vondruš <mosra@centrum.cz>
#       2017 — Jonathan Hale <squareys@googlemail.com>
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
project(MagnumAreaLightsExample)

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
find_package(MagnumExtras REQUIRED Ui)

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(MagnumPlugins REQUIRED
        DdsImporter
        StbTrueTypeFont)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(AreaLights_RESOURCES resources.conf)

add_executable(magnum-arealights AreaLightsExample.cpp ${AreaLights_RESOURCES})
target_link_libraries(magnum-arealights PRIVATE
    Magnum::Application
    Magnum::Magnum
    Magnum::Shaders
    MagnumExtras::Ui)

if(CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(magnum-arealights PRIVATE
        MagnumPlugins::DdsImporter
        MagnumPlugins::StbTrueTypeFont)
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES arealights.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/arealights RENAME index.html)
    install(TARGETS magnum-arealights DESTINATION ${MAGNUM_DEPLOY_PREFIX}/arealights)
    install(FILES
        ${CMAKE_SOURCE_DIR}/src/WebApplication.css
        ${CMAKE_SOURCE_DIR}/src/EmscriptenApplication.js
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/arealights)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-arealights.js.mem
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-arealights.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/arealights OPTIONAL)

# Installation for desktop
else()
    install(TARGETS magnum-arealights DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()
