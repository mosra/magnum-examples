#.rst:
# Find Magnum extras
# ------------------
#
# Finds Magnum extras. Basic usage::
#
#  find_package(MagnumExtras REQUIRED)
#
# This command tries to find Magnum extras and then defines the following:
#
#  MagnumExtras_FOUND       - Whether Magnum extras were found
#
# This command alone is useless without specifying the components:
#
#  Ui                       - Ui library
#  ui-gallery               - magnum-ui-gallery executable
#  player                   - magnum-player executable
#
# Example usage with specifying additional components is:
#
#  find_package(MagnumExtras REQUIRED Ui)
#
# For each component is then defined:
#
#  MagnumExtras_*_FOUND     - Whether the component was found
#  MagnumExtras::*          - Component imported target
#
# The package is found if either debug or release version of each requested
# library is found. If both debug and release libraries are found, proper
# version is chosen based on actual build configuration of the project (i.e.
# Debug build is linked to debug libraries, Release build to release
# libraries).
#
# Additionally these variables are defined for internal usage:
#
#  MAGNUMEXTRAS_INCLUDE_DIR - Magnum extras include dir (w/o
#   dependencies)
#  MAGNUMEXTRAS_*_LIBRARY_DEBUG - Debug version of given library, if found
#  MAGNUMEXTRAS_*_LIBRARY_RELEASE - Release version of given library, if
#   found
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021 Vladimír Vondruš <mosra@centrum.cz>
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

# Corrade library dependencies
set(_MAGNUMEXTRAS_CORRADE_DEPENDENCIES )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    if(_component STREQUAL Ui)
        set(_MAGNUMEXTRAS_${_component}_CORRADE_DEPENDENCIES Interconnect)
    endif()

    list(APPEND _MAGNUMEXTRAS_CORRADE_DEPENDENCIES ${_MAGNUMEXTRAS_${_component}_CORRADE_DEPENDENCIES})
endforeach()
find_package(Corrade REQUIRED ${_MAGNUMEXTRAS_CORRADE_DEPENDENCIES})

# Magnum library dependencies
set(_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    if(_component STREQUAL Ui)
        set(_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES Text GL)
    endif()

    list(APPEND _MAGNUMEXTRAS_MAGNUM_DEPENDENCIES ${_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES})

# Global integration include dir
find_path(MAGNUMEXTRAS_INCLUDE_DIR Magnum
    HINTS ${MAGNUM_INCLUDE_DIR})
mark_as_advanced(MAGNUMEXTRAS_INCLUDE_DIR)

# Component distinction (listing them explicitly to avoid mistakes with finding
# components from other repositories)
set(_MAGNUMEXTRAS_LIBRARY_COMPONENTS Ui)
set(_MAGNUMEXTRAS_EXECUTABLE_COMPONENTS player ui-gallery)
# Nothing is enabled by default right now
set(_MAGNUMEXTRAS_IMPLICITLY_ENABLED_COMPONENTS )

# Inter-component dependencies
set(_MAGNUMEXTRAS_ui-gallery_DEPENDENCIES Ui)

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    # Mark the dependencies as required if the component is also required
    if(MagnumExtras_FIND_REQUIRED_${_component})
        foreach(_dependency ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
            set(MagnumExtras_FIND_REQUIRED_${_dependency} TRUE)
        endforeach()
    endif()

    list(APPEND _MAGNUMEXTRAS_ADDITIONAL_COMPONENTS ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
set(_MAGNUMEXTRAS_ORIGINAL_FIND_COMPONENTS ${MagnumExtras_FIND_COMPONENTS})
if(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumExtras_FIND_COMPONENTS 0 ${_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS})
endif()
if(MagnumExtras_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumExtras_FIND_COMPONENTS)
endif()

# Additional components
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    # Create imported target in case the library is found. If the project is
    # added as subproject to CMake, the target already exists and all the
    # required setup is already done from the build tree.
    if(TARGET MagnumExtras::${_component})
        set(MagnumExtras_${_component}_FOUND TRUE)
    else()
        # Library components
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            add_library(MagnumExtras::${_component} UNKNOWN IMPORTED)

            # Try to find both debug and release version
            find_library(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG Magnum${_component}-d)
            find_library(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE Magnum${_component})
            mark_as_advanced(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG
                MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE)

            if(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE)
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
                set_property(TARGET MagnumExtras::${_component} PROPERTY
                    IMPORTED_LOCATION_RELEASE ${MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE})
            endif()

            if(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG)
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
                set_property(TARGET MagnumExtras::${_component} PROPERTY
                    IMPORTED_LOCATION_DEBUG ${MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG})
            endif()

        # Executables
        elseif(_component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS)
            add_executable(MagnumExtras::${_component} IMPORTED)

            find_program(MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE magnum-${_component})
            mark_as_advanced(MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE)

            if(MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE)
                set_property(TARGET MagnumExtras::${_component} PROPERTY
                    IMPORTED_LOCATION ${MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE})
            endif()

        # Something unknown, skip. FPHSA will take care of handling this below.
        else()
            continue()
        endif()

        # No special setup required for Ui library

        # Find library includes
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            find_path(_MAGNUMEXTRAS_${_COMPONENT}_INCLUDE_DIR
                NAMES ${_component}.h
                HINTS ${MAGNUMEXTRAS_INCLUDE_DIR}/Magnum/${_component})
            mark_as_advanced(_MAGNUMEXTRAS_${_COMPONENT}_INCLUDE_DIR)
        endif()

        # Link to core Magnum library, add inter-library dependencies
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_CORRADE_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Corrade::${_dependency})
            endforeach()
            set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Magnum::Magnum)
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Magnum::${_dependency})
            endforeach()

            # Add inter-project dependencies
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES MagnumExtras::${_dependency})
            endforeach()
        endif()

        # Decide if the library was found
        if((_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS AND _MAGNUMEXTRAS_${_COMPONENT}_INCLUDE_DIR AND (MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG OR MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE)) OR (_component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS AND MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE))
            set(MagnumExtras_${_component}_FOUND TRUE)
        else()
            set(MagnumExtras_${_component}_FOUND FALSE)
        endif()
    endif()
endforeach()

# For CMake 3.16+ with REASON_FAILURE_MESSAGE, provide additional potentially
# useful info about the failed components.
if(NOT CMAKE_VERSION VERSION_LESS 3.16)
    set(_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE )
    # Go only through the originally specified find_package() components, not
    # the dependencies added by us afterwards
    foreach(_component ${_MAGNUMEXTRAS_ORIGINAL_FIND_COMPONENTS})
        if(MagnumExtras_${_component}_FOUND)
            continue()
        endif()

        # If it's not known at all, tell the user -- it might be a new library
        # and an old Find module, or something platform-specific.
        if(NOT _component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS AND NOT _component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS)
            list(APPEND _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_component} is not a known component on this platform.")
        # Otherwise, if it's not among implicitly built components, hint that
        # the user may need to enable it
        # TODO: currently, the _FOUND variable doesn't reflect if dependencies
        #   were found. When it will, this needs to be updated to avoid
        #   misleading messages.
        elseif(NOT _component IN_LIST _MAGNUMEXTRAS_IMPLICITLY_ENABLED_COMPONENTS)
            string(TOUPPER ${_component} _COMPONENT)
            list(APPEND _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_component} is not built by default. Make sure you enabled WITH_${_COMPONENT} when building Magnum Extras.")
        # Otherwise we have no idea. Better be silent than to print something
        # misleading.
        else()
        endif()
    endforeach()

    string(REPLACE ";" " " _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE}")
    set(_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE REASON_FAILURE_MESSAGE "${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumExtras
    REQUIRED_VARS MAGNUMEXTRAS_INCLUDE_DIR
    HANDLE_COMPONENTS
    ${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE})
