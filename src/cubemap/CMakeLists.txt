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
project(MagnumCubeMapExample)

# CMake policies: don't treat imported targets with :: as files
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

find_package(Magnum REQUIRED
    MeshTools
    Primitives
    SceneGraph
    Sdl2Application)

if(CORRADE_TARGET_IOS)
    find_package(MagnumPlugins REQUIRED StbImageImporter)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(CubeMap_RESOURCES resources.conf)

set(CubeMap_SRCS
    CubeMap.cpp
    CubeMapExample.cpp
    CubeMapShader.cpp
    Reflector.cpp
    ReflectorShader.cpp
    Types.cpp)
set(CubeMap_HEADERS
    CubeMap.h
    CubeMapShader.h
    Reflector.h
    ReflectorShader.h
    Types.h)

if(CORRADE_TARGET_IOS)
    set(CubeMap_BUNDLE
        +x.jpg
        +y.jpg
        +z.jpg
        -x.jpg
        -y.jpg
        -z.jpg)
    set_source_files_properties(${CubeMap_BUNDLE} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND CubeMap_SRCS ${CubeMap_BUNDLE})
endif()

add_executable(magnum-cubemap
    ${CubeMap_SRCS}
    ${CubeMap_HEADERS}
    ${CubeMap_RESOURCES})
target_include_directories(magnum-cubemap PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(magnum-cubemap
    Magnum::Application
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph)

# iOS-specific
if(CORRADE_TARGET_IOS)
    target_link_libraries(magnum-cubemap MagnumPlugins::StbImageImporter)
    set_target_properties(magnum-cubemap PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
endif()

# Installation for desktop
if(NOT CORRADE_TARGET_IOS)
    install(TARGETS magnum-cubemap DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
    install(FILES README.md DESTINATION ${MAGNUM_DATA_INSTALL_DIR}/examples RENAME README-cubemap.md)
    install(FILES +x.jpg +y.jpg +z.jpg -x.jpg -y.jpg -z.jpg DESTINATION ${MAGNUM_DATA_INSTALL_DIR}/examples/cubemap)
endif()
