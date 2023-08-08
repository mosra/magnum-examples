#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
#       2019 — Nghia Truong <nghiatruong.vn@gmail.com>
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

project(FluidSimulation2DExample CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    GL
    MeshTools
    Primitives
    SceneGraph
    Shaders
    Sdl2Application)
find_package(MagnumIntegration REQUIRED ImGui)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(FluidSimulation2D_RESOURCES resources.conf)

add_executable(magnum-fluidsimulation2d WIN32
    FluidSimulation2DExample.cpp
    DataStructures/Array2X.h
    DataStructures/MathHelpers.h
    DataStructures/PCGSolver.h
    DataStructures/SDFObject.h
    DataStructures/SparseMatrix.h
    DrawableObjects/FlatShadeObject2D.h
    DrawableObjects/ParticleGroup2D.h
    DrawableObjects/ParticleGroup2D.cpp
    DrawableObjects/WireframeObject2D.h
    FluidSolver/SolverData.h
    FluidSolver/ApicSolver2D.h
    FluidSolver/ApicSolver2D.cpp
    Shaders/ParticleSphereShader2D.h
    Shaders/ParticleSphereShader2D.cpp
    ${FluidSimulation2D_RESOURCES})
target_link_libraries(magnum-fluidsimulation2d PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    MagnumIntegration::ImGui)
target_include_directories(magnum-fluidsimulation2d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS magnum-fluidsimulation2d DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-fluidsimulation2d)

#group into MagnumExamples on visual studio
set_target_properties(magnum-fluidsimulation2d PROPERTIES FOLDER "MagnumExamples")