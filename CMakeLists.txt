#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023, 2024, 2025
#            — Vladimír Vondruš <mosra@centrum.cz>
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

# Matches Corrade requirement, see its root CMakeLists for more information.
cmake_minimum_required(VERSION 3.5...3.10)

# CMake 3.12+ uses the policy max version specified in
# cmake_minimum_required(), meaning that with ...3.10, everything until CMP0071
# gets set to NEW implicitly. We however want to keep compatibility with
# versions before 3.12, so the NEW policies are still being hand-picked. Also
# don't want to do a blanket cmake_policy(VERSION) because that may break
# behavior for existing projects that rely on the OLD behavior.

# Don't restrict INTERPROCEDURAL_OPTIMIZATION only for icc on Linux
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()
# If CMAKE_AUTOMOC is set, all uses of corrade_add_resource() would otherwise
# complain on 3.10 that AUTOMOC is not processing GENERATED files
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
# Allow <PackageName>_ROOT to be used on 3.12+ to point to per-package install
# locations that find_package(PackageName) subsequently picks up
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
# Allow also <PACKAGENAME>_ROOT (i.e., uppercase), on 3.27+
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
# Superprojects can use just set(MAGNUM_WITH_BLAH ON) without FORCE CACHE on
# 3.13+
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

project(MagnumExamples CXX)

# Use folders for nice tree in Visual Studio and XCode
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

# If Magnum is not found, fetch all dependencies directly so people can clone &
# build the examples directly. Only on CMake 3.28+ tho so we can use
# EXCLUDE_FROM_ALL, SYSTEM and fallback to find_package().
if(CMAKE_VERSION VERSION_LESS 3.28)
    find_package(Magnum REQUIRED)
elseif(NOT MAGNUM_BUILD_SELFCONTAINED)
    # Find Corrade first, and only if found try to find Magnum. Do it this way
    # because FindMagnum.cmake internally has find_package(Corrade REQUIRED)
    # which would make the process fail with error instead of falling back to
    # fetching them.
    # TODO fix in the Find modules themselves, same for FindMagnumPlugins etc.
    find_package(Corrade)
    if(Corrade_FOUND)
        find_package(Magnum)
    endif()
endif()
if(NOT CMAKE_VERSION VERSION_LESS 3.28 AND (MAGNUM_BUILD_SELFCONTAINED OR (NOT Magnum_FOUND AND NOT DEFINED MAGNUM_BUILD_SELFCONTAINED)))
    # If a self-contained build wasn't explicitly requested, print a message to
    # let the user know and explicitly set the option to not have the message
    # printed every time.
    if(NOT DEFINED MAGNUM_BUILD_SELFCONTAINED)
        set(MAGNUM_BUILD_SELFCONTAINED ON CACHE BOOL "")
        message(WARNING "Magnum not found, downloading and building a local copy along with other dependencies. Pass -DMAGNUM_BUILD_SELFCONTAINED=OFF to CMake during the initial run to disable this fallback.")
    endif()

    include(FetchContent)
    # Note that while CMAKE_ARGS is supported by ExternalProject_Add(), it
    # isn't by FetchContent_Declare() and have to set the options as regular
    # varibles, with caveats, because ugh of course nothing is ever simple.
    #   https://discourse.cmake.org/t/fetchcontent-declare-with-cmake-args-does-not-work/11227/5

    FetchContent_Declare(
        Corrade
        GIT_REPOSITORY https://github.com/mosra/corrade
        # No GIT_TAG, picking latest master
        EXCLUDE_FROM_ALL
        # This makes the thing try find_package(Corrade) first, and not
        # download anything if found. In particular, the find_package(Magnum)
        # above might find Corrade (and thus make its Corrade:: targets
        # available) but not Magnum. Having Corrade then downloaded and added
        # as a subproject would cause the Corrade:: targets to conflict.
        #
        # Also requiring at least the Utility component, as otherwise it'll
        # succeed as soon as add_subdirectory(corrade) is done at least once
        # due to Corrade's CMakeLists setting INTERNAL CACHE variables for
        # FindCorrade to work when Corrade is a subproject. Which means, it'd
        # do the right thing in the initial run, but in subsequent runs
        # find_package(Corrade) alone would succeed, making FetchContent think
        # the package is available in the system, resulting in it skipping the
        # add_subdirectory(corrade) altogether, resulting in Corrade not being
        # actually made available at all.
        FIND_PACKAGE_ARGS COMPONENTS Utility)
    # Finally, FetchContent_MakeAvailable() overrides find_package(Corrade)
    # with some no-op stupidity. That we *do not* want because FindCorrade is
    # populating essential variables such as CORRADE_TARGET_*.
    file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/CorradeExtra.cmake
        "include(\${corrade_SOURCE_DIR}/modules/FindCorrade.cmake)\n")

    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG 5d249570393f7a37e037abf22cd6012a4cc56a71  # release-2.32.10
        EXCLUDE_FROM_ALL
        SYSTEM
        # This makes the thing try find_package(SDL2) first, and not download
        # anything if found
        FIND_PACKAGE_ARGS )
    # Keep in sync with what's in Sdl2Application docs. The CACHE BOOL "" FORCE
    # is still needed because SDL itself doesn't enable the CMP0077 policy.
    set(SDL_ATOMIC OFF CACHE BOOL "" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_CPUINFO OFF CACHE BOOL "" FORCE)
    set(SDL_EVENT OFF CACHE BOOL "" FORCE)
    set(SDL_FILE OFF CACHE BOOL "" FORCE)
    set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set(SDL_LOCALE OFF CACHE BOOL "" FORCE)
    set(SDL_POWER OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        Magnum
        GIT_REPOSITORY https://github.com/mosra/magnum
        # No GIT_TAG, picking latest master
        EXCLUDE_FROM_ALL)
    # Compared to Corrade, Magnum doesn't have FIND_PACKAGE_ARGS so we don't
    # need to provide the Extra.cmake module to redirect back to FindMagnum.
    set(MAGNUM_WITH_SDL2APPLICATION ON)
    if(MAGNUM_WITH_AREALIGHTS_EXAMPLE OR # TODO drop once Ui has icon font
       MAGNUM_WITH_CUBEMAP_EXAMPLE OR
       MAGNUM_WITH_VIEWER_EXAMPLE)
        set(MAGNUM_WITH_ANYIMAGEIMPORTER ON)
    endif()
    if(MAGNUM_WITH_VIEWER_EXAMPLE)
        set(MAGNUM_WITH_ANYSCENEIMPORTER ON)
    endif()
    if(MAGNUM_WITH_TEXTUREDQUAD_EXAMPLE)
        set(MAGNUM_WITH_TGAIMPORTER ON)
    endif()

    # These need to be listed in dependency order
    FetchContent_MakeAvailable(Corrade SDL2 Magnum)

    # Include also Magnum Plugins if these examples are enabled
    if(MAGNUM_WITH_ANIMATED_GIF_EXAMPLE OR
       MAGNUM_WITH_AREALIGHTS_EXAMPLE OR
       MAGNUM_WITH_CUBEMAP_EXAMPLE OR
       MAGNUM_WITH_TEXT_EXAMPLE OR
       MAGNUM_WITH_VIEWER_EXAMPLE)
        FetchContent_Declare(
            MagnumPlugins
            GIT_REPOSITORY https://github.com/mosra/magnum-plugins
            # No GIT_TAG, picking latest master
            EXCLUDE_FROM_ALL)
        # Compared to Corrade, Magnum doesn't have FIND_PACKAGE_ARGS so we
        # don't need to provide the Extra.cmake module to redirect back to
        # FindMagnumPlugins. Moreover, nothing actually uses find_package()
        # for the plugins anyway.
        if(MAGNUM_WITH_AREALIGHTS_EXAMPLE)
            set(MAGNUM_WITH_DDSIMPORTER ON)
        endif()
        if(MAGNUM_WITH_VIEWER_EXAMPLE)
            set(MAGNUM_WITH_GLTFIMPORTER ON)
        endif()
        if(MAGNUM_WITH_AREALIGHTS_EXAMPLE OR # TODO drop once Ui has icon font
           MAGNUM_WITH_ANIMATED_GIF_EXAMPLE OR
           MAGNUM_WITH_CUBEMAP_EXAMPLE OR
           MAGNUM_WITH_VIEWER_EXAMPLE)
            set(MAGNUM_WITH_STBIMAGEIMPORTER ON)
        endif()
        if(MAGNUM_WITH_AREALIGHTS_EXAMPLE OR
           MAGNUM_WITH_TEXT_EXAMPLE)
            set(MAGNUM_WITH_STBTRUETYPEFONT ON)
        endif()
        FetchContent_MakeAvailable(MagnumPlugins)
    endif()

    # Include also magnum-extras for these examples
    if(MAGNUM_WITH_AREALIGHTS_EXAMPLE)
        FetchContent_Declare(
            MagnumExtras
            GIT_REPOSITORY https://github.com/mosra/magnum-extras
            # No GIT_TAG, picking latest master
            EXCLUDE_FROM_ALL)
        # Compared to Corrade, Magnum doesn't have FIND_PACKAGE_ARGS so we
        # don't need to provide the Extra.cmake module to redirect back to
        # FindMagnumExtras.
        set(MAGNUM_WITH_UI ON)
        FetchContent_MakeAvailable(MagnumExtras)
    endif()

    # Include also ImGui and magnum-integration for these examples
    if(MAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE OR
       MAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE OR
       MAGNUM_WITH_IMGUI_EXAMPLE)
        FetchContent_Declare(
            ImGui
            GIT_REPOSITORY https://github.com/ocornut/imgui
            # TODO bump to a newer version once
            # https://github.com/mosra/magnum-integration/pull/119 is merged
            GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06) # v1.91.9b
        # Compared to Corrade, Magnum doesn't have FIND_PACKAGE_ARGS so we
        # don't need to provide the Extra.cmake module to redirect back to
        # FindMagnumExtras.
        FetchContent_Declare(
            MagnumIntegration
            GIT_REPOSITORY https://github.com/mosra/magnum-integration
            # No GIT_TAG, picking latest master
            EXCLUDE_FROM_ALL)

        # The imgui_SOURCE_DIR variable is only set once
        # FetchContent_MakeAvailable() is called, so cannot do it together with
        # MagnumIntegration below
        FetchContent_MakeAvailable(ImGui)
        set(IMGUI_DIR ${imgui_SOURCE_DIR})
        set(MAGNUM_WITH_IMGUIINTEGRATION ON)
        FetchContent_MakeAvailable(MagnumIntegration)
    endif()
endif()

include(CMakeDependentOption)

# Options that used to be unprefixed. 29 values for 29 option() and
# cmake_dependent_option() calls. New options shouldn't be added to this list.
set(_MAGNUMEXAMPLES_DEPRECATED_UNPREFIXED_OPTIONS
    WITH_ARCBALL_EXAMPLE
    WITH_ANIMATED_GIF_EXAMPLE
    WITH_AREALIGHTS_EXAMPLE
    WITH_AUDIO_EXAMPLE
    WITH_BOX2D_EXAMPLE
    WITH_BULLET_EXAMPLE
    WITH_CUBEMAP_EXAMPLE
    WITH_DART_EXAMPLE
    WITH_FLUIDSIMULATION2D_EXAMPLE
    WITH_FLUIDSIMULATION3D_EXAMPLE
    WITH_IMGUI_EXAMPLE
    WITH_LEAPMOTION_EXAMPLE
    WITH_MOUSEINTERACTION_EXAMPLE
    WITH_MOTIONBLUR_EXAMPLE
    WITH_OCTREE_EXAMPLE
    WITH_OVR_EXAMPLE
    WITH_PICKING_EXAMPLE
    WITH_PRIMITIVES_EXAMPLE
    WITH_RAYTRACING_EXAMPLE
    WITH_SHADOWS_EXAMPLE
    WITH_TEXT_EXAMPLE
    WITH_TEXTUREDQUAD_EXAMPLE
    WITH_TEXTUREDTRIANGLE_VULKAN_EXAMPLE
    WITH_TRIANGLE_EXAMPLE
    WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE
    WITH_TRIANGLE_SOKOL_EXAMPLE
    WITH_TRIANGLE_VULKAN_EXAMPLE
    WITH_VIEWER_EXAMPLE
    WITH_WEBXR_EXAMPLE
    FLUIDSIMULATION3D_EXAMPLE_USE_MULTITHREADING
    FLUIDSIMULATION3D_EXAMPLE_USE_TBB)
# If during the first run (i.e., when the variable isn't in cache yet), check
# if any of the prefixed options are already set. If so, we assume the user is
# already switched to the prefixed options and won't accept the deprecated
# unprefixed options for backwards compatibility. This way it's possible for
# projects to reuse these variables for other purposes without affecting
# Corrade in any way.
if(NOT DEFINED _MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
    foreach(option ${_MAGNUMEXAMPLES_DEPRECATED_UNPREFIXED_OPTIONS})
        if(DEFINED MAGNUM_${option})
            set(_MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS OFF CACHE INTERNAL "")
            break()
        endif()
    endforeach()
endif()

option(MAGNUM_WITH_ARCBALL_EXAMPLE "Build ArcBall Camera example" OFF)
option(MAGNUM_WITH_ANIMATED_GIF_EXAMPLE "Build Animated Gif example" OFF)
option(MAGNUM_WITH_AREALIGHTS_EXAMPLE "Build Area Lights example (requires the Ui library and some TTF font plugin)" OFF)
option(MAGNUM_WITH_AUDIO_EXAMPLE "Build Audio example (requires the Audio library and the StbVorbisAudioImporter plugin)" OFF)
option(MAGNUM_WITH_BOX2D_EXAMPLE "Build Box2D integration example" OFF)
option(MAGNUM_WITH_BULLET_EXAMPLE "Build Bullet integration example (requires the BulletIntegration library)" OFF)
cmake_dependent_option(MAGNUM_WITH_CUBEMAP_EXAMPLE "Build CubeMap example (requires some JPEG importer plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(MAGNUM_WITH_DART_EXAMPLE "Build DART integration example (requires the DartIntegration library)" OFF)
option(MAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE "Build 2D Fluid Simulation example (requires the ImGui integration)" OFF)
option(MAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE "Build 3D Fluid Simulation example (requires the ImGui integration)" OFF)
option(MAGNUM_WITH_IMGUI_EXAMPLE "Build ImGui example" OFF)
option(MAGNUM_WITH_LEAPMOTION_EXAMPLE "Build LeapMotion example" OFF)
option(MAGNUM_WITH_MOUSEINTERACTION_EXAMPLE "Build Mouse Interaction example" OFF)
cmake_dependent_option(MAGNUM_WITH_MOTIONBLUR_EXAMPLE "Build Motion Blur example" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(MAGNUM_WITH_OCTREE_EXAMPLE "Build Octree example" OFF)
option(MAGNUM_WITH_OVR_EXAMPLE "Build OVR example" OFF)
option(MAGNUM_WITH_PICKING_EXAMPLE "Build Picking example" OFF)
option(MAGNUM_WITH_PONG_EXAMPLE "Build Pong example" ON)
option(MAGNUM_WITH_PRIMITIVES_EXAMPLE "Build Primitives example" OFF)
option(MAGNUM_WITH_RAYTRACING_EXAMPLE "Build Ray Tracing example" OFF)
option(MAGNUM_WITH_SHADOWS_EXAMPLE "Build Shadow Mapping example" OFF)
option(MAGNUM_WITH_TEXT_EXAMPLE "Build Text example (requires some TTF font plugin)" OFF)
cmake_dependent_option(MAGNUM_WITH_TEXTUREDQUAD_EXAMPLE "Build Textured Quad example (requires some TGA importer plugin)" OFF "NOT MAGNUM_TARGET_GLES" OFF)
option(MAGNUM_WITH_TEXTUREDTRIANGLE_VULKAN_EXAMPLE "Build Vulkan Textured Triangle example (requires some TGA importer plugin)" OFF)
option(MAGNUM_WITH_TRIANGLE_EXAMPLE "Build Triangle example" ON)
option(MAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE "Build Plain GLFW Triangle example" OFF)
option(MAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE "Build sokol_gfx Triangle example" OFF)
option(MAGNUM_WITH_TRIANGLE_VULKAN_EXAMPLE "Build Vulkan Triangle example" OFF)
option(MAGNUM_WITH_VIEWER_EXAMPLE "Build Viewer example (requires the AnySceneImporter plugin)" OFF)
if(CORRADE_TARGET_EMSCRIPTEN)
    option(MAGNUM_WITH_WEBXR_EXAMPLE "Build WebXR example" OFF)
endif()

# Backwards compatibility for unprefixed CMake options. If the user isn't
# explicitly using prefixed options in the first run already, accept the
# unprefixed options, and remember this decision for subsequent runs
if(NOT DEFINED _MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
endif()
# If the user wasn't explicitly using prefixed options in the first run and the
# MAGNUM_BUILD_DEPRECATED option is not currently disabled (which can get
# changed subsequently), accept the unprefixed options and print a warning if
# they're different from the prefixed ones.
if(_MAGNUMEXAMPLES_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS AND MAGNUM_BUILD_DEPRECATED)
    set(_MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION )
    foreach(option ${_MAGNUMEXAMPLES_DEPRECATED_UNPREFIXED_OPTIONS})
        if(DEFINED ${option})
            # CMake has no comparison of boolean values (EQUAL returns false if
            # comparing ON and 1 or OFF and FALSE, STREQUAL also), so we have
            # to do it this way. Also warn only on the first encountered
            # variable so people can fix it, reconfigure and go to the next one
            # that warns.
            if((${option} AND NOT MAGNUM_${option}) OR
               (NOT ${option} AND MAGNUM_${option}) AND NOT _MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION)
                set(_MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION ${option})
            endif()
            set(MAGNUM_${option} ${${option}})
            # If variables specified on the command line don't match any
            # options, they're kept in cache but set as UNINITIALIZED, meaning
            # they don't appear in cmake-gui or ccmake, so there's no way to
            # fix the warning apart from hand-enditing the CMakeCache.txt or
            # recreating the build dir. Update their cached type to be BOOL to
            # make them appear.
            set(${option} ${${option}} CACHE BOOL "Deprecated, use MAGNUM_${option} instead" FORCE)
        endif()
    endforeach()

    if(_MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION)
        message(DEPRECATION "Unprefixed options such as ${_MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION} are deprecated, use MAGNUM_${_MAGNUMEXAMPLES_WARN_DEPRECATED_UNPREFIXED_OPTION} instead. Delete the unprefixed variable from CMake cache or set both to the same value to silence this warning.")
    endif()
endif()

# Deprecated options
if(MAGNUM_BUILD_DEPRECATED AND WITH_TEXTUREDTRIANGLE_EXAMPLE AND NOT WITH_TEXTUREDQUAD_EXAMPLE AND NOT MAGNUM_WITH_TEXTUREDQUAD_EXAMPLE)
    set(MAGNUM_WITH_TEXTUREDQUAD_EXAMPLE ON)
    message(DEPRECATION "The WITH_TEXTUREDTRIANGLE_EXAMPLE option is deprecated. Use MAGNUM_WITH_TEXTUREDQUAD_EXAMPLE instead.")
endif()

# A single output location. After a decade of saying NO THIS IS A NON-SOLUTION
# TO A NON-PROBLEM I reconsidered my views and enabled this, because:
#
# - On Windows (which don't have RPATH), this makes test execution finally
#   possible without having to install all the stuff first (including the
#   test-only libs, which is ugh).
# - With CMake subprojects, this makes it finally possible to use dynamic
#   plugins directly from the build dir (again without installing anything) ---
#   all plugins are put into the same place, so PluginManager has a single
#   place to look into; and thanks to the dynamic libraries being there as
#   well, this location can be automagically detected as relative to
#   Utility::Path::libraryLocation().
# - Thanks to the $<CONFIG> being part of the output path, you are always sure
#   you never accidentally mix up debug/release libraries when switching
#   CMAKE_BUILD_TYPE in an existing build dir.
#
# The runtime location is set to CMAKE_BINARY_DIR and not PROJECT_BINARY_DIR
# because have one runtime location per CMake subproject would not solve much
# either. If the user already provides CMAKE_RUNTIME_OUTPUT_DIRECTORY (even
# empty), it's respected and nothing is being done.
#
# Explicitly using a generator expression to ensure plugins are added to e.g.
# <CONFIG>/lib/magnum/importers/ instead of lib/magnum/importers/<CONFIG>. Also
# adding this to cache, making superprojects pick that up implicitly as well,
# without forcing them to explicitly mirror this setting.
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    # There should be no need for the "90% use case" user to adjust these, so
    # don't show them in the default view
    mark_as_advanced(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        CMAKE_LIBRARY_OUTPUT_DIRECTORY
        CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
endif()

add_subdirectory(src)
