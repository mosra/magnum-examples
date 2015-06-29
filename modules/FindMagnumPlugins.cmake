# - Find Magnum plugins
#
# Basic usage:
#  find_package(MagnumPlugins [REQUIRED])
# This command tries to find Magnum plugins and then defines:
#  MAGNUMPLUGINS_FOUND          - Whether Magnum plugins were found
# This command will not try to find any actual plugin. The plugins are:
#  AnyImageImporter - Any image importer
#  AnySceneImporter - Any scene importer
#  ColladaImporter  - Collada importer
#  FreeTypeFont     - FreeType font
#  HarfBuzzFont     - HarfBuzz font
#  JpegImporter     - JPEG importer
#  OpenGexImporter  - OpenGEX importer
#  PngImporter      - PNG importer
#  StanfordImporter - Stanford PLY importer
#  StbImageImporter - Image importer using stb_image
#  StbPngImageConverter - PNG image converter using stb_image_write
# Example usage with specifying the plugins is:
#  find_package(MagnumPlugins [REQUIRED|COMPONENTS]
#               FreeTypeFont PngImporter)
# For each plugin is then defined:
#  MAGNUMPLUGINS_*_FOUND        - Whether the plugin was found
#  MAGNUMPLUGINS_*_LIBRARIES    - Plugin library and dependent libraries
#  MAGNUMPLUGINS_*_INCLUDE_DIRS - Include dirs of plugin dependencies
#
# The package is found if either debug or release version of each requested
# plugin is found. If both debug and release plugins are found, proper version
# is chosen based on actual build configuration of the project (i.e. Debug
# build is linked to debug plugins, Release build to release plugins). See
# FindMagnum.cmake for more information about autodetection of
# MAGNUM_PLUGINS_DIR.
#
# Additionally these variables are defined for internal usage:
#  MAGNUMPLUGINS_*_LIBRARY      - Plugin library (w/o dependencies)
#  MAGNUMPLUGINS_*_LIBRARY_DEBUG - Debug version of given library, if found
#  MAGNUMPLUGINS_*_LIBRARY_RELEASE - Release version of given library, if found
#

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

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMPLUGINS_ADDITIONAL_COMPONENTS )
foreach(component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # The dependencies need to be sorted by their dependency order as well
    if(component STREQUAL ColladaImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter)
    elseif(component STREQUAL OpenGexImporter)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES AnyImageImporter)
    elseif(component STREQUAL HarfBuzzFont)
        set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES FreeTypeFont)
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

# Magnum library dependencies
set(_MAGNUMPLUGINS_DEPENDENCIES )
foreach(component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    if(component MATCHES ".+AudioImporter$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCY Audio)
    elseif(component MATCHES ".+(Font|FontConverter)$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCY Text)
    endif()

    list(APPEND _MAGNUMPLUGINS_DEPENDENCIES ${_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCY})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMPLUGINS_DEPENDENCIES})

# Additional components
foreach(component ${MagnumPlugins_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # Plugin library suffix
    if(${component} MATCHES ".+AudioImporter$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX audioimporters)
    elseif(${component} MATCHES ".+Importer$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX importers)
    elseif(${component} MATCHES ".+Font$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX fonts)
    elseif(${component} MATCHES ".+ImageConverter$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX imageconverters)
    elseif(${component} MATCHES ".+FontConverter$")
        set(_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX fontconverters)
    endif()

    # Find the library. Dynamic plugins don't have any prefix (e.g. `lib` on
    # Linux), search with empty prefix and then reset that back so we don't
    # accidentaly break something else
    set(_tmp_prefixes ${CMAKE_FIND_LIBRARY_PREFIXES})
    set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")

    # Try to find both debug and release version. Dynamic and static debug
    # libraries are on different places.
    find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG ${component}
        PATH_SUFFIXES magnum-d/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})
    find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG ${component}-d
        PATH_SUFFIXES magnum/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})
    find_library(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE ${component}
        PATH_SUFFIXES magnum/${_MAGNUMPLUGINS_${_COMPONENT}_PATH_SUFFIX})

    # Set the _LIBRARY variable based on what was found
    if(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG AND MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE)
        set(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY
            debug ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG}
            optimized ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE})
    elseif(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG)
        set(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG})
    elseif(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE)
        set(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE})
    endif()

    set(CMAKE_FIND_LIBRARY_PREFIXES ${_tmp_prefixes})

    # Find include path
    find_path(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR
            NAMES ${component}.h
            PATHS ${MAGNUM_INCLUDE_DIR}/MagnumPlugins/${component})

    # AnyImageImporter has no dependencies
    # AnySceneImporter has no dependencies

    # ColladaImporter plugin dependencies
    if(${component} STREQUAL ColladaImporter)
        find_package(Qt4)
        if(QT4_FOUND)
            set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTXMLPATTERNS_LIBRARY})
            set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${QT_INCLUDE_DIR})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endif()

    # FreeTypeFont plugin dependencies
    if(${component} STREQUAL FreeTypeFont)
        find_package(Freetype)
        if(FREETYPE_FOUND)
            set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${FREETYPE_LIBRARIES})
            set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endif()

    # HarfBuzzFont plugin dependencies
    if(${component} STREQUAL HarfBuzzFont)
        find_package(Freetype)
        find_package(HarfBuzz)
        if(FREETYPE_FOUND AND HARFBUZZ_FOUND)
            set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${FREETYPE_LIBRARIES} ${HARFBUZZ_LIBRARIES})
            set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS} ${HARFBUZZ_INCLUDE_DIRS})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endif()

    # JpegImporter plugin dependencies
    if(${component} STREQUAL JpegImporter)
        find_package(JPEG)
        if(JPEG_FOUND)
            set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${JPEG_LIBRARIES})
            set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endif()

    # OpenGexImporter has no dependencies

    # PngImporter plugin dependencies
    if(${component} STREQUAL PngImporter)
        find_package(PNG)
        if(PNG_FOUND)
            set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${PNG_LIBRARIES})
            set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${PNG_INCLUDE_DIRS})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endif()

    # StanfordImporter has no dependencies
    # StbImageImporter has no dependencies
    # StbPngImageConverter has no dependencies

    # Add Magnum library dependency, if there is any
    if(_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCY)
        string(TOUPPER ${_MAGNUMPLUGINS_${_COMPONENT}_MAGNUM_DEPENDENCY} _DEPENDENCY)
        set(_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES ${_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES} ${MAGNUM_${_DEPENDENCY}_LIBRARIES})
        set(_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS ${_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS} ${MAGNUM_${_DEPENDENCY}_INCLUDE_DIRS})
    endif()

    # Add inter-project dependencies, mark the component as not found if
    # any dependency is not found
    set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_LIBRARIES )
    set(_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_INCLUDE_DIRS )
    foreach(dependency ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCIES})
        string(TOUPPER ${dependency} _DEPENDENCY)
        if(MAGNUMPLUGINS_${_DEPENDENCY}_LIBRARY)
            list(APPEND _MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_LIBRARIES ${MAGNUMPLUGINS_${_DEPENDENCY}_LIBRARY} ${_MAGNUMPLUGINS_${_DEPENDENCY}_LIBRARIES})
            list(APPEND _MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_INCLUDE_DIRS ${_MAGNUMPLUGINS_${_DEPENDENCY}_INCLUDE_DIRS})
        else()
            unset(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY)
        endif()
    endforeach()

    # Decide if the plugin was found
    if(MAGNUMPLUGINS_${_COMPONENT}_LIBRARY AND _MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR)
        set(MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES
            ${MAGNUMPLUGINS_${_COMPONENT}_LIBRARY}
            ${_MAGNUMPLUGINS_${_COMPONENT}_LIBRARIES}
            ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_LIBRARIES}
            ${MAGNUM_LIBRARIES})
        set(MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS
            ${_MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIRS}
            ${_MAGNUMPLUGINS_${_COMPONENT}_DEPENDENCY_INCLUDE_DIRS})

        set(MagnumPlugins_${component}_FOUND TRUE)

        # Don't expose variables w/o dependencies to end users
        mark_as_advanced(FORCE
            MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_DEBUG
            MAGNUMPLUGINS_${_COMPONENT}_LIBRARY_RELEASE
            MAGNUMPLUGINS_${_COMPONENT}_LIBRARY
            _MAGNUMPLUGINS_${_COMPONENT}_INCLUDE_DIR)
    else()
        set(MagnumPlugins_${component}_FOUND FALSE)
    endif()
endforeach()

# We need to feed FPHSA with at least one variable in REQUIRED_VARS (its value
# is then displayed in the "Found" message)
include(FindPackageHandleStandardArgs)
set(_MAGNUMPLUGINS_INCLUDE_DIR ${MAGNUM_INCLUDE_DIR}/MagnumPlugins)
find_package_handle_standard_args(MagnumPlugins
    REQUIRED_VARS _MAGNUMPLUGINS_INCLUDE_DIR
    HANDLE_COMPONENTS)
