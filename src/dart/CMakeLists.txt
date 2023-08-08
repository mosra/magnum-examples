#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
#       2019 — Konstantinos Chatzilygeroudis <costashatz@gmail.com>
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

cmake_minimum_required(VERSION 3.5)

project(MagnumDartExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../../modules/")
endif()

set(DART_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/urdf)
set(DART_EXAMPLE_INSTALL_DIR ${MAGNUM_DATA_INSTALL_DIR}/examples/dart)

# We need the URDF loader of DART for this example
find_package(DART 6.0.0 CONFIG REQUIRED OPTIONAL_COMPONENTS io-urdf utils-urdf)

if(NOT (DART_utils-urdf_FOUND OR DART_io-urdf_FOUND) OR (DART_VERSION STRLESS 6.0.0))
    message(FATAL_ERROR "DART version >= 6 and DART urdf library are required for this example!")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    GL
    MeshTools
    Primitives
    SceneGraph
    Sdl2Application
    Shaders
    Trade)
find_package(MagnumIntegration REQUIRED Dart)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(magnum-dart WIN32 DartExample.cpp)
target_include_directories(magnum-dart PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(magnum-dart PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::MeshTools
    Magnum::Magnum
    Magnum::SceneGraph
    Magnum::Primitives
    Magnum::Shaders
    MagnumIntegration::Dart
    dart)
if(DART_utils-urdf_FOUND)
    target_link_libraries(magnum-dart PRIVATE dart-utils-urdf)
elseif(DART_io-urdf_FOUND)
    target_link_libraries(magnum-dart PRIVATE dart-io-urdf)
endif()

install(TARGETS magnum-dart DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
install(FILES
    urdf/iiwa14/link_0.stl
    urdf/iiwa14/link_1.stl
    urdf/iiwa14/link_2.stl
    urdf/iiwa14/link_3.stl
    urdf/iiwa14/link_4.stl
    urdf/iiwa14/link_5.stl
    urdf/iiwa14/link_6.stl
    urdf/iiwa14/link_7.stl
    DESTINATION ${DART_EXAMPLE_INSTALL_DIR}/iiwa14)
install(FILES
    urdf/robotiq/inner_finger_fine.stl
    urdf/robotiq/inner_knuckle_fine.stl
    urdf/robotiq/outer_finger_fine.stl
    urdf/robotiq/outer_knuckle_fine.stl
    urdf/robotiq/robotiq_85_base_link_fine.stl
    DESTINATION ${DART_EXAMPLE_INSTALL_DIR}/robotiq)
install(FILES
    urdf/iiwa14_simple.urdf
    urdf/robotiq.urdf
    DESTINATION ${DART_EXAMPLE_INSTALL_DIR})

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-dart)

#group into MagnumExamples on visual studio
set_target_properties(magnum-dart PROPERTIES FOLDER "MagnumExamples")