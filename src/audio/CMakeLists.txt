#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023, 2024, 2025
#            — Vladimír Vondruš <mosra@centrum.cz>
#       2015, 2017 — Jonathan Hale <squareys@googlemail.com>
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

cmake_minimum_required(VERSION 3.5...3.10)

project(MagnumAudioExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    Audio
    GL
    MeshTools
    Primitives
    SceneGraph
    Shaders
    Trade)

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication)
    find_package(MagnumPlugins REQUIRED StbVorbisAudioImporter)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(AudioExample_RESOURCES resources.conf)

add_executable(magnum-audio WIN32
    AudioExample.cpp
    ${AudioExample_RESOURCES})
target_link_libraries(magnum-audio PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::Audio
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade)

if(CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(magnum-audio PRIVATE
        MagnumPlugins::StbVorbisAudioImporter)
endif()

# For Emscripten copy the boilerplate next to the executable so it can be run
# directly from the build dir; provide an install target as well
if(CORRADE_TARGET_EMSCRIPTEN)
    add_custom_command(TARGET magnum-audio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            ${MAGNUM_WEBAPPLICATION_CSS}
            $<TARGET_FILE_DIR:magnum-audio>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/audio.html
            $<TARGET_FILE_DIR:magnum-audio>/magnum-audio.html)

    install(FILES audio.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/audio RENAME index.html)
    install(TARGETS magnum-audio DESTINATION ${MAGNUM_DEPLOY_PREFIX}/audio)
    install(FILES
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/audio)
    install(FILES
        $<TARGET_FILE_DIR:magnum-audio>/magnum-audio.js.mem
        $<TARGET_FILE_DIR:magnum-audio>/magnum-audio.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/audio OPTIONAL)

# Installation for desktop
else()
    install(TARGETS magnum-audio DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-audio)
