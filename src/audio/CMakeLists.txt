#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2015 Jonathan Hale <squareys@googlemail.com>
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

cmake_minimum_required(VERSION 2.8.12)
project(MagnumAudioExample)

find_package(Magnum REQUIRED
    Audio
    DebugTools
    SceneGraph
    Sdl2Application)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORRADE_CXX_FLAGS}")

include_directories(${MAGNUM_INCLUDE_DIRS} ${MAGNUM_APPLICATION_INCLUDE_DIRS} ${MAGNUM_AUDIO_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

corrade_add_resource(AudioData resources.conf)

add_executable(magnum-audio
    AudioExample.cpp

    ${AudioData})

target_link_libraries(magnum-audio
    ${MAGNUM_LIBRARIES}
    ${MAGNUM_AUDIO_LIBRARIES}
    ${MAGNUM_SCENEGRAPH_LIBRARIES}
    ${MAGNUM_DEBUGTOOLS_LIBRARIES}
    ${MAGNUM_APPLICATION_LIBRARIES})

install(TARGETS magnum-audio DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
install(FILES README.md DESTINATION ${MAGNUM_DATA_INSTALL_DIR}/examples RENAME README-audio.md)
