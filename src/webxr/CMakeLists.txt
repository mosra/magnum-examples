#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
#           Vladimír Vondruš <mosra@centrum.cz>
#       2019 — Jonathan Hale <squareys@googlemail.com>
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

project(MagnumWebXrExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Magnum REQUIRED
    GL
    MeshTools
    Primitives
    Shaders
    EmscriptenApplication)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

# The library_webxr.js and webxr.h files are bundled from
# https://github.com/VhiteRabbit/emscripten-webxr/tree/e101ce7cfc3c92962a65771f43c2d61775edd348

# Closure Compiler chokes on the JS file, disable it for this example
string(REPLACE "--closure 1" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

add_executable(magnum-webxr WebXrExample.cpp)
target_link_libraries(magnum-webxr PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::Shaders)
set_property(TARGET magnum-webxr APPEND_STRING PROPERTY LINK_FLAGS
    "--js-library ${CMAKE_CURRENT_SOURCE_DIR}/library_webxr.js")

# Copy the boilerplate next to the executable so it can be run directly from
# the build dir; provide an install target as well
add_custom_command(TARGET magnum-webxr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        $<TARGET_FILE_DIR:magnum-webxr>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/webxr.html
        $<TARGET_FILE_DIR:magnum-webxr>/magnum-webxr.html)

install(FILES webxr.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/webxr RENAME index.html)
install(TARGETS magnum-webxr DESTINATION ${MAGNUM_DEPLOY_PREFIX}/webxr)
install(FILES
    ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
    ${MAGNUM_WEBAPPLICATION_CSS}
    DESTINATION ${MAGNUM_DEPLOY_PREFIX}/webxr)
install(FILES
    $<TARGET_FILE_DIR:magnum-webxr>/magnum-webxr.js.mem
    $<TARGET_FILE_DIR:magnum-webxr>/magnum-webxr.wasm
    DESTINATION ${MAGNUM_DEPLOY_PREFIX}/webxr OPTIONAL)
