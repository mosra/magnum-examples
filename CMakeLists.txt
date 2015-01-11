#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015
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
project(MagnumExamples)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")
find_package(Magnum REQUIRED)

include(CMakeDependentOption)
option(WITH_BULLET "Build Bullet integration example (requires BulletIntegration library)" OFF)
cmake_dependent_option(WITH_CUBEMAP "Build CubeMap example (requires JpegImporter plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
cmake_dependent_option(WITH_FRAMEBUFFER "Build Framebuffer example (requires TgaImporter plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
cmake_dependent_option(WITH_MOTIONBLUR "Build MotionBlur example" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(WITH_PRIMITIVES "Build Primitives example" OFF)
option(WITH_TEXT "Build Text example (requires FreeTypeFont plugin)" OFF)
cmake_dependent_option(WITH_TEXTUREDTRIANGLE "Build TexturedTriangle example (requires TgaImporter plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(WITH_TRIANGLE "Build Triangle example" ON)
option(WITH_VIEWER "Build Viewer example (requires ColladaImporter plugin)" OFF)

add_subdirectory(src)
