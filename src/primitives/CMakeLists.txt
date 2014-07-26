#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014
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

cmake_minimum_required(VERSION 2.8.8)
project(MagnumPrimitivesExample)

find_package(Magnum REQUIRED
    MeshTools
    Primitives
    Shaders)

if(CORRADE_TARGET_NACL)
    find_package(Magnum REQUIRED NaClApplication)
elseif(CORRADE_TARGET_ANDROID)
    find_package(Magnum REQUIRED AndroidApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORRADE_CXX_FLAGS}")
include_directories(${MAGNUM_INCLUDE_DIRS} ${MAGNUM_APPLICATION_INCLUDE_DIRS})

if(NOT CORRADE_TARGET_ANDROID)
    add_executable(primitives PrimitivesExample.cpp)
else()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}")
    add_library(primitives SHARED PrimitivesExample.cpp)
endif()
target_link_libraries(primitives
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_MESHTOOLS_LIBRARIES}
    ${MAGNUM_PRIMITIVES_LIBRARIES}
    ${MAGNUM_SHADERS_LIBRARIES}
    ${MAGNUM_APPLICATION_LIBRARIES})

# Installation for NaCl
if(CORRADE_TARGET_NACL)
    if(NOT CORRADE_TARGET_NACL_NEWLIB)
        message(FATAL_ERROR "Only newlib NaCl target is supported")
    endif()
    install(FILES primitives-nacl.html primitives-nacl.nmf DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(TARGETS primitives DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# Installation for Emscripten
if(CORRADE_TARGET_EMSCRIPTEN)
    install(FILES primitives-emscripten.html DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(TARGETS primitives DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
