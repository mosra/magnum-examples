#.rst:
# Find Magnum plugins
# -------------------
#
# Finds the Magnum plugins library. Basic usage::
#
#  find_package(MagnumPlugins REQUIRED)
#
# This command tries to find Magnum plugins and then defines the following:
#
#  MagnumPlugins_FOUND          - Whether Magnum plugins were found
#
# This command will not try to find any actual plugin. The plugins are:
#
#  AnyAudioImporter             - Any audio importer
#  AnyImageConverter            - Any image converter
#  AnyImageImporter             - Any image importer
#  AnySceneImporter             - Any scene importer
#  AssimpImporter               - Assimp importer
#  ColladaImporter              - Collada importer
#  DdsImporter                  - DDS importer
#  DevIlImageImporter           - Image importer using DevIL
#  DrFlacAudioImporter          - FLAC audio importer plugin using dr_flac
#  DrWavAudioImporter           - WAV audio importer plugin using dr_wav
#  FreeTypeFont                 - FreeType font
#  HarfBuzzFont                 - HarfBuzz font
#  JpegImporter                 - JPEG importer
#  MiniExrImageConverter        - OpenEXR image converter using miniexr
#  OpenGexImporter              - OpenGEX importer
#  PngImageConverter            - PNG image converter
#  PngImporter                  - PNG importer
#  StanfordImporter             - Stanford PLY importer
#  StbImageConverter            - Image converter using stb_image_write
#  StbImageImporter             - Image importer using stb_image
#  StbTrueTypeFont              - TrueType font using stb_truetype
#  StbVorbisAudioImporter       - OGG audio importer using stb_vorbis
#  TinyGltfImporter             - GLTF importer using tiny_gltf
#
# Example usage with specifying the plugins is::
#
#  find_package(MagnumPlugins REQUIRED FreeTypeFont PngImporter)
#
# For each plugin is then defined:
#
#  MagnumPlugins_*_FOUND        - Whether the plugin was found
#  MagnumPlugins::*             - Plugin imported target
#
# The package is found if either debug or release version of each requested
# plugin is found. If both debug and release plugins are found, proper version
# is chosen based on actual build configuration of the project (i.e. ``Debug``
# build is linked to debug plugins, ``Release`` build to release plugins). See
# ``FindMagnum.cmake`` for more information about autodetection of
# ``MAGNUM_PLUGINS_DIR``.
#
# Additionally these variables are defined for internal usage:
#
#  MAGNUMPLUGINS_INCLUDE_DIR    - Magnum plugins include dir (w/o dependencies)
#  MAGNUMPLUGINS_*_LIBRARY      - Plugin library (w/o dependencies)
#  MAGNUMPLUGINS_*_LIBRARY_DEBUG - Debug version of given library, if found
#  MAGNUMPLUGINS_*_LIBRARY_RELEASE - Release version of given library, if found
#
# Workflows without imported targets are deprecated and the following variables
# are included just for backwards compatibility and only if
# :variable:`MAGNUM_BUILD_DEPRECATED` is enabled:
#
#  MAGNUMPLUGINS_*_LIBRARIES    - Expands to ``MagnumPlugins::*` target. Use
#   ``MagnumPlugins::*`` target directly instead.
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

# Magnum library dependencies
set(_MAGNUMPLUGINS_DEPENDENCIES )
foreach(_component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    if(_component MATCHES ".+AudioImporter$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCIES Audio)
    elseif(_component MATCHES ".+(Font|FontConverter)$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCIES Text)
    endif()

    if(_component STREQUAL ColladaImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCIES MeshTools)
    endif()

    list(APPEND _MAGNUMPLUGINS_DEPENDENCIES ${_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCIES})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMPLUGINS_DEPENDENCIES})

# Global plugin include dir
find_path(MAGNUMPLUGINS_INCLUDE_DIR MagnumPlugins
    HINTS ${MAGNUM_INCLUDE_DIR})
mark_as_advanced(MAGNUMPLUGINS_INCLUDE_DIR)

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMPLUGINS_ADDITIONAL_COMPONENTS )
foreach(_component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    if(_component STREQUAL AssimpImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter)
    elseif(_component STREQUAL ColladaImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter)
    elseif(_component STREQUAL OpenGexImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter)
    elseif(_component STREQUAL TinyGltfImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter StbImageImporter)
    elseif(_component STREQUAL HarfBuzzFont)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES FreeTypeFont)
    endif()

    # Mark the dependencies as required if the component is also required
    if(MagnumPlugins_FIND_REQUIRED_${_component})
        foreach(_dependency ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES})
            set(MagnumPlugins_FIND_REQUIRED_${_dependency} TRUE)
        endforeach()
    endif()

    list(APPEND _MAGNUMPLUGINS_ADDITIONAL_COMPONENTS ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
if(_MAGNUMPLUGINS_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumPlugins_FIND_COMPONENTS 0 ${_MAGNUMPLUGINS_ADDITIONAL_COMPONENTS})
endif()
if(MagnumPlugins_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumPlugins_FIND_COMPONENTS)
endif()

# Component distinction (listing them explicitly to avoid mistakes with finding
# components from other repositories)
set(_MAGNUMPLUGINS_PLUGIN_COMPONENTS "^(AnyAudioImporter|AnyImageConverter|AnyImageImporter|AnySceneImporter|AssimpImporter|ColladaImporter|DdsImporter|DevIlImageImporter|DrFlacAudioImporter|DrWavAudioImporter|FreeTypeFont|HarfBuzzFont|JpegImporter|MiniExrImageConverter|OpenGexImporter|PngImageConverter|PngImporter|StanfordImporter|StbImageConverter|StbImageImporter|StbTrueTypeFont|StbVorbisAudioImporter|TinyGltfImporter)$")

# Find all components
foreach(_component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    # Create imported target in case the library is found. If the project is
    # added as subproject to CMake, the target already exists and all the
    # required setup is already done from the build tree.
    if(TARGET MagnumPlugins::${_component})
        set(MagnumPlugins_${_component}_FOUND TRUE)
    else()
        # Plugin components
        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS})
            add_library(MagnumPlugins::${_component} UNKNOWN IMPORTED)

            # AudioImporter plugin specific name suffixes
            if(_component MATCHES ".+AudioImporter$")
                set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX audioimporters)

                # Audio importer class is Audio::*Importer, thus we need to
                # convert *AudioImporter.h to *Importer.h
                string(REPLACE "AudioImporter" "Importer" _MAGNUMPLUGINS_${_COMPONENT}_HEADER_NAME "${_component}")
                set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_PATH_NAMES ${_MAGNUMPLUGINS_${_COMPONENT}_HEADER_NAME}.h)

            # Importer plugin specific name suffixes
            elseif(_component MATCHES ".+Importer$")
                set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX importers)

            # Font plugin specific name suffixes
            elseif(_component MATCHES ".+Font$")
                set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX fonts)

            # ImageConverter plugin specific name suffixes
            elseif(_component MATCHES ".+ImageConverter$")
                set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX imageconverters)

            # FontConverter plugin specific name suffixes
            elseif(_component MATCHES ".+FontConverter$")
                set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX fontconverters)
            endif()

            # Don't override the exception for *AudioImporter plugins
            if(NOT _MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_PATH_NAMES)
                set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_PATH_NAMES ${_component}.h)
            endif()

            # Dynamic plugins don't have any prefix (e.g. `lib` on Linux),
            # search with empty prefix and then reset that back so we don't
            # accidentaly break something else
            set(_tmp_prefixes "${CMAKE_FIND_LIBRARY_PREFIXES}")
            set(CMAKE_FIND_LIBRARY_PREFIXES "${CMAKE_FIND_LIBRARY_PREFIXES};")

            # Try to find both debug and release version. Dynamic and static
            # debug libraries are on different places.
            find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG ${_component}
                PATH_SUFFIXES magnum-d/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})
            find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG ${_component}-d
                PATH_SUFFIXES magnum/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})
            find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE ${_component}
                PATH_SUFFIXES magnum/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})
            mark_as_advanced(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG
                MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE)

            # Reset back
            set(CMAKE_FIND_LIBRARY_PREFIXES "${_tmp_prefixes}")
        endif()

        # Library location for libraries/plugins
        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS})
            if(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
                set_property(TARGET MagnumPlugins::${_component} PROPERTY
                    IMPORTED_LOCATION_RELEASE ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE})
            endif()

            if(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
                set_property(TARGET MagnumPlugins::${_component} PROPERTY
                    IMPORTED_LOCATION_DEBUG ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG})
            endif()
        endif()

        # AnyAudioImporter has no dependencies
        # AnyImageImporter has no dependencies
        # AnySceneImporter has no dependencies

        # AssimpImporter plugin dependencies
        if(_component STREQUAL AssimpImporter)
            find_package(AssimpImporter)
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Assimp::Assimp)
        endif()

        # ColladaImporter plugin dependencies
        if(_component STREQUAL ColladaImporter)
            find_package(Qt4)
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES ${QT_INCLUDE_DIR})
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTXMLPATTERNS_LIBRARY})
        endif()

        # DdsImporter has no dependencies

        # DevIlImageImporter plugin dependencies
        if(_component STREQUAL DevIlImageImporter)
            find_package(DevIL)
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ${IL_LIBRARIES} ${ILU_LIBRARIES})
        endif()

        # DrFlacAudioImporter has no dependencies
        # DrWavAudioImporter has no dependencies

        # FreeTypeFont plugin dependencies
        if(_component STREQUAL FreeTypeFont)
            find_package(Freetype)
            # Need to handle special cases where both debug and release
            # libraries are available (in form of debug;A;optimized;B in
            # FREETYPE_LIBRARIES), thus appending them one by one
            if(FREETYPE_LIBRARY_DEBUG AND FREETYPE_LIBRARY_RELEASE)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "$<$<NOT:$<CONFIG:Debug>>:${FREETYPE_LIBRARY_RELEASE}>;$<$<CONFIG:Debug>:${FREETYPE_LIBRARY_DEBUG}>")
            else()
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ${FREETYPE_LIBRARIES})
            endif()
        endif()

        # HarfBuzzFont plugin dependencies
        if(_component STREQUAL HarfBuzzFont)
            find_package(Freetype)
            find_package(HarfBuzz)
            # Need to handle special cases where both debug and release
            # libraries are available (in form of debug;A;optimized;B in
            # FREETYPE_LIBRARIES), thus appending them one by one
            if(FREETYPE_LIBRARY_DEBUG AND FREETYPE_LIBRARY_RELEASE)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "$<$<NOT:$<CONFIG:Debug>>:${FREETYPE_LIBRARY_RELEASE}>;$<$<CONFIG:Debug>:${FREETYPE_LIBRARY_DEBUG}>")
            else()
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ${FREETYPE_LIBRARIES})
            endif()
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ${HARFBUZZ_LIBRARIES})
        endif()

        # JpegImporter plugin dependencies
        if(_component STREQUAL JpegImporter)
            find_package(JPEG)
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ${JPEG_LIBRARIES})
        endif()

        # MiniExrImageConverter has no dependencies
        # OpenGexImporter has no dependencies

        # PngImageConverter plugin dependencies
        if(_component STREQUAL PngImageConverter)
            find_package(PNG)
            # Need to handle special cases where both debug and release
            # libraries are available (in form of debug;A;optimized;B in
            # PNG_LIBRARIES), thus appending them one by one
            if(PNG_LIBRARY_DEBUG AND PNG_LIBRARY_RELEASE)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "$<$<NOT:$<CONFIG:Debug>>:${PNG_LIBRARY_RELEASE}>;$<$<CONFIG:Debug>:${PNG_LIBRARY_DEBUG}>")
            else()
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ${PNG_LIBRARIES})
            endif()
        endif()

        # PngImporter plugin dependencies
        if(_component STREQUAL PngImporter)
            find_package(PNG)
            # Need to handle special cases where both debug and release
            # libraries are available (in form of debug;A;optimized;B in
            # PNG_LIBRARIES), thus appending them one by one
            if(PNG_LIBRARY_DEBUG AND PNG_LIBRARY_RELEASE)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "$<$<NOT:$<CONFIG:Debug>>:${PNG_LIBRARY_RELEASE}>;$<$<CONFIG:Debug>:${PNG_LIBRARY_DEBUG}>")
            else()
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ${PNG_LIBRARIES})
            endif()
        endif()

        # StanfordImporter has no dependencies
        # StbImageConverter has no dependencies
        # StbImageImporter has no dependencies
        # StbTrueTypeFont has no dependencies
        # StbVorbisAudioImporter has no dependencies
        # TinyGltfImporter has no dependencies

        # Find plugin includes
        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS})
            find_path(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR
                NAMES ${_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_PATH_NAMES}
                HINTS ${MAGNUMPLUGINS_INCLUDE_DIR}/MagnumPlugins/${_component})
            mark_as_advanced(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR)
        endif()

        # Automatic import of static plugins on CMake >= 3.1
        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS} AND NOT CMAKE_VERSION VERSION_LESS 3.1)
            file(READ ${_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR}/configure.h _magnumPlugins${_component}Configure)
            string(FIND "${_magnumPlugins${_component}Configure}" "#define MAGNUM_${_COMPONENT}_BUILD_STATIC" _magnumPlugins${_component}_BUILD_STATIC)
            if(NOT _magnumPlugins${_component}_BUILD_STATIC EQUAL -1)
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_SOURCES ${_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR}/importStaticPlugin.cpp)
            endif()
        endif()

        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS})
            # Link to core Magnum library, add other Magnum dependencies
            set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Magnum::Magnum)
            foreach(_dependency ${_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCIES})
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Magnum::${_dependency})
            endforeach()

            # Add inter-project dependencies
            foreach(_dependency ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES})
                set_property(TARGET MagnumPlugins::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES MagnumPlugins::${_dependency})
            endforeach()
        endif()

        # Decide if the plugin was found
        if(_component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS} AND _MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR AND (MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG OR MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE))
            set(MagnumPlugins_${_component}_FOUND TRUE)
        else()
            set(MagnumPlugins_${_component}_FOUND FALSE)
        endif()
    endif()

    # Deprecated variables
    if(MAGNUM_BUILD_DEPRECATED AND _component MATCHES ${_MAGNUMPLUGINS_PLUGIN_COMPONENTS})
        set(MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES MagnumPlugins::${_component})
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumPlugins
    REQUIRED_VARS MAGNUMPLUGINS_INCLUDE_DIR
    HANDLE_COMPONENTS)
