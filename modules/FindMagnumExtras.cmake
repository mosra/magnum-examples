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
#  Octree - Octree BSP structure for Magnum::SceneGraph
#
# Example usage with specifying additional components is:
#
#  find_package(MagnumExtras REQUIRED SomeLibraryThatDoesntExistYet)
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
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2016 Jonathan Hale <squareys@googlemail.com>
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
    string(TOUPPER ${_component} _COMPONENT)

    # (none yet)

    list(APPEND _MAGNUMEXTRAS_CORRADE_DEPENDENCIES ${_MAGNUMEXTRAS_${_COMPONENT}_CORRADE_DEPENDENCIES})
endforeach()
find_package(Corrade REQUIRED ${_MAGNUMEXTRAS_CORRADE_DEPENDENCIES})

# Magnum library dependencies
set(_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    if(_component MATCHES Octree)
        set(_MAGNUMEXTRAS_${_COMPONENT}_MAGNUM_DEPENDENCIES Shapes)
    endif()

    if(_component MATCHES SceneGraph)
        set(_MAGNUMEXTRAS_${_COMPONENT}_MAGNUM_DEPENDENCIES SceneGraph)
    endif()

    list(APPEND _MAGNUMEXTRAS_MAGNUM_DEPENDENCIES ${_MAGNUMEXTRAS_${_COMPONENT}_MAGNUM_DEPENDENCIES})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES})

# Global integration include dir
find_path(MAGNUMEXTRAS_INCLUDE_DIR Magnum
    HINTS ${MAGNUM_INCLUDE_DIR})
mark_as_advanced(MAGNUMEXTRAS_INCLUDE_DIR)

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    # (no inter-component dependencies yet)
    if(_component MATCHES SceneGraph)
        set(_MAGNUMEXTRAS_${_COMPONENT}_DEPENDENCIES Octree)
    endif()

    # Mark the dependencies as required if the component is also required
    if(MagnumExtras_FIND_REQUIRED_${_component})
        foreach(_dependency ${})
            set(MagnumExtras_FIND_REQUIRED_${_dependency} TRUE)
        endforeach()
    endif()

    list(APPEND _MAGNUMEXTRAS_ADDITIONAL_COMPONENTS ${_MAGNUMEXTRAS_${_COMPONENT}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
if(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumExtras_FIND_COMPONENTS 0 ${_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS})
endif()
if(MagnumExtras_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumExtras_FIND_COMPONENTS)
endif()

# Component distinction (listing them explicitly to avoid mistakes with finding
# components from other repositories)
set(_MAGNUMEXTRAS_LIBRARY_COMPONENTS "(Octree|SceneGraph)")

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
        if(_component MATCHES ${_MAGNUMEXTRAS_LIBRARY_COMPONENTS})
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
        endif()

        # (none yet)

        # Find library includes
        if(_component MATCHES ${_MAGNUMEXTRAS_LIBRARY_COMPONENTS})
            find_path(_MAGNUMEXTRAS_${_COMPONENT}_INCLUDE_DIR
                NAMES ${_component}.h
                HINTS ${MAGNUMEXTRAS_INCLUDE_DIR}/Magnum/${_component})
        endif()

        if(_component MATCHES ${_MAGNUMEXTRAS_LIBRARY_COMPONENTS})
            # Link to Corrade dependencies, link to core Magnum library and
            # other Magnum dependencies
            foreach(_dependency ${_MAGNUMEXTRAS_${_COMPONENT}_CORRADE_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Corrade::${_dependency})
            endforeach()
            set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Magnum::Magnum)
            foreach(_dependency ${_MAGNUMEXTRAS_${_COMPONENT}_MAGNUM_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Magnum::${_dependency})
            endforeach()

            # Add inter-project dependencies
            foreach(_dependency ${_MAGNUMEXTRAS_${_COMPONENT}_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES MagnumExtras::${_dependency})
            endforeach()
        endif()

        # Decide if the library was found
        if(_component MATCHES ${_MAGNUMEXTRAS_LIBRARY_COMPONENTS} AND _MAGNUMEXTRAS_${_COMPONENT}_INCLUDE_DIR AND (MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG OR MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE))
            set(MagnumExtras_${_component}_FOUND TRUE)
        else()
            set(MagnumExtras_${_component}_FOUND FALSE)
        endif()
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumExtras
    REQUIRED_VARS MAGNUMEXTRAS_INCLUDE_DIR
    HANDLE_COMPONENTS)
