#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 2.8.9)
project(MagnumCubeMapExample)

find_package(Magnum REQUIRED
    MeshTools
    Primitives
    SceneGraph
    Sdl2Application)

if(CORRADE_TARGET_IOS)
    find_package(MagnumPlugins REQUIRED StbImageImporter)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORRADE_CXX_FLAGS}")
include_directories(${MAGNUM_INCLUDE_DIRS} ${MAGNUM_APPLICATION_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

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
target_link_libraries(magnum-cubemap
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES}
    ${MAGNUM_APPLICATION_LIBRARIES})

# iOS-specific
if(CORRADE_TARGET_IOS)
    target_link_libraries(magnum-cubemap ${MAGNUMPLUGINS_STBIMAGEIMPORTER_LIBRARIES})
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
