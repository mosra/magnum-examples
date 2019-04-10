#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

# Configuration fails on < 3.4 if only C++ is enabled (CheckFunctionExists
# macro called from FindX11)
if(NOT CMAKE_VERSION VERSION_LESS 3.4.0)
    set(LANG CXX)
endif()
project(MagnumExamples ${LANG})

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})
find_package(Magnum REQUIRED)

include(CMakeDependentOption)
option(WITH_AREALIGHTS_EXAMPLE "Build Area Lights example (requires the Ui library and some TTF font plugin)" OFF)
option(WITH_AUDIO_EXAMPLE "Build Audio example (requires the Audio library and the StbVorbisAudioImporter plugin)" OFF)
option(WITH_BOX2D_EXAMPLE "Build Box2D integration example" OFF)
option(WITH_BULLET_EXAMPLE "Build Bullet integration example (requires the BulletIntegration library)" OFF)
cmake_dependent_option(WITH_CUBEMAP_EXAMPLE "Build CubeMap example (requires some JPEG importer plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(WITH_IMGUI_EXAMPLE "Build ImGui example" OFF)
option(WITH_LEAPMOTION_EXAMPLE "Build LeapMotion example" OFF)
option(WITH_MOUSEINTERACTION_EXAMPLE "Build Mouse Interaction example" OFF)
cmake_dependent_option(WITH_MOTIONBLUR_EXAMPLE "Build Motion Blur example" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(WITH_OVR_EXAMPLE "Build OVR example" OFF)
option(WITH_PICKING_EXAMPLE "Build Picking example" OFF)
option(WITH_PRIMITIVES_EXAMPLE "Build Primitives example" OFF)
option(WITH_SHADOWS_EXAMPLE "Build Shadow Mapping example" OFF)
option(WITH_TEXT_EXAMPLE "Build Text example (requires some TTF font plugin)" OFF)
cmake_dependent_option(WITH_TEXTUREDTRIANGLE_EXAMPLE "Build TexturedTriangle example (requires some TGA importer plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(WITH_TRIANGLE_EXAMPLE "Build Triangle example" ON)
option(WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE "Build Plain GLFW Triangle example" OFF)
option(WITH_TRIANGLE_SOKOL_EXAMPLE "Build sokol_gfx Triangle example" OFF)
option(WITH_TRIANGLE_VULKAN_EXAMPLE "Build Vulkan Triangle example" OFF)
option(WITH_VIEWER_EXAMPLE "Build Viewer example (requires the AnySceneImporter plugin)" OFF)
if(CORRADE_TARGET_EMSCRIPTEN)
    option(WITH_WEBVR_EXAMPLE "Build WebVR example" OFF)
endif()

add_subdirectory(src)
