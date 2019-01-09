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

cmake_minimum_required(VERSION 3.1)

project(MagnumTextExample)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../../modules/")
endif()

find_package(Magnum REQUIRED
    GL
    Shaders
    Text
    Trade
    Sdl2Application)

if(MAGNUM_BUILD_STATIC)
    find_package(Magnum REQUIRED TgaImporter MagnumFont)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(TextExample_RESOURCES resources.conf)

add_executable(magnum-text
    TextExample.cpp
    ${TextExample_RESOURCES})
target_link_libraries(magnum-text PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Text
    Magnum::Trade)

if(MAGNUM_BUILD_STATIC)
    target_link_libraries(magnum-text PRIVATE
        Magnum::MagnumFont)
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES text.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/text RENAME index.html)
    install(TARGETS magnum-text DESTINATION ${MAGNUM_DEPLOY_PREFIX}/text)
    install(FILES
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/text)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-text.js.mem
        ${CMAKE_CURRENT_BINARY_DIR}/magnum-text.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/text OPTIONAL)

# Installation for desktop
else()
    install(TARGETS magnum-text DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()
