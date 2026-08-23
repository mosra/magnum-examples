#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
#            — Vladimír Vondruš <mosra@centrum.cz>
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

project(MagnumMultiDrawExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    DebugTools
    GL
    MeshTools
    Shaders
    SceneGraph
    SceneTools
    Trade)
find_package(MagnumExtras REQUIRED Ui)
if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication)
    find_package(MagnumPlugins REQUIRED
        GltfImporter
        StbTrueTypeFont)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(magnum-multidraw WIN32 MultiDrawExample.cpp)
if(CORRADE_TARGET_EMSCRIPTEN)
    corrade_add_resource(MagnumMultiDraw_RESOURCES resources.conf)
    target_sources(magnum-multidraw PRIVATE ${MagnumMultiDraw_RESOURCES})
endif()

target_link_libraries(magnum-multidraw PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::DebugTools
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::SceneTools
    Magnum::Shaders
    Magnum::Trade
    MagnumExtras::Ui)
if(CORRADE_TARGET_EMSCRIPTEN)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
    endif()
    target_link_libraries(magnum-multidraw PRIVATE
        MagnumPlugins::GltfImporter
        MagnumPlugins::StbTrueTypeFont)
    target_link_options(magnum-multidraw PRIVATE
        "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    add_custom_command(TARGET magnum-multidraw POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MAGNUM_WEBAPPLICATION_CSS}
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            $<TARGET_FILE_DIR:magnum-multidraw>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/multidraw.html"
            "$<TARGET_FILE_DIR:magnum-multidraw>/magnum-multidraw.html")
    install(FILES multidraw.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-multidraw RENAME index.html)
    install(TARGETS magnum-multidraw DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-multidraw)
    install(FILES
        $<TARGET_FILE_DIR:magnum-multidraw>/magnum-multidraw.js.mem
        $<TARGET_FILE_DIR:magnum-multidraw>/magnum-multidraw.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-multidraw OPTIONAL)
    install(FILES
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-multidraw)
else()
    install(TARGETS magnum-multidraw DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-multidraw)
