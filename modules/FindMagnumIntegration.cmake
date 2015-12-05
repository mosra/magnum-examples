# - Find Magnum integration library
#
# Basic usage:
#  find_package(MagnumIntegration [REQUIRED])
# This command tries to find Magnum integration library and then defines:
#  MAGNUMINTEGRATION_FOUND      - Whether the library was found
#  MAGNUMINTEGRATION_INCLUDE_DIRS - Magnum integration include dir and include
#   dirs of global dependencies
# This command alone is useless without specifying the components:
#  Bullet                       - Bullet Physics integration library
#  Ovr                          - Oculus SDK integration library
# Example usage with specifying additional components is:
#  find_package(MagnumIntegration [REQUIRED|COMPONENTS]
#               Bullet)
# For each component is then defined:
#  MAGNUM_*INTEGRATION_FOUND    - Whether the component was found
#  MAGNUM_*INTEGRATION_LIBRARIES - Component library and dependent libraries
#  MAGNUM_*INTEGRATION_INCLUDE_DIRS - Include dirs of dependencies
#
# The package is found if either debug or release version of each requested
# library is found. If both debug and release libraries are found, proper
# version is chosen based on actual build configuration of the project (i.e.
# Debug build is linked to debug libraries, Release build to release
# libraries).
#
# Additionally these variables are defined for internal usage:
#  MAGNUMINTEGRATION_INCLUDE_DIR - Magnum integration include dir (w/o
#   dependencies)
#  MAGNUM_*INTEGRATION_LIBRARY  - Component library (w/o dependencies)
#  MAGNUM_*INTEGRATION_LIBRARY_DEBUG - Debug version of given library, if found
#  MAGNUM_*INTEGRATION_LIBRARY_RELEASE - Release version of given library, if
#   found
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
set(_MAGNUMINTEGRATION_ADDITIONAL_COMPONENTS )
foreach(component ${MagnumIntegration_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # The dependencies need to be sorted by their dependency order as well
    # (no inter-component dependencies yet)

    list(APPEND _MAGNUMINTEGRATION_ADDITIONAL_COMPONENTS ${_MAGNUM_${_COMPONENT}INTEGRATION_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
if(_MAGNUMINTEGRATION_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumIntegration_FIND_COMPONENTS 0 ${_MAGNUMINTEGRATION_ADDITIONAL_COMPONENTS})
endif()
if(MagnumIntegration_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumIntegration_FIND_COMPONENTS)
endif()

# Magnum library dependencies
set(_MAGNUMINTEGRATION_DEPENDENCIES )
foreach(component ${MagnumIntegration_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    if(component STREQUAL Bullet)
        # SceneGraph is implicit Shapes dependency, fugly hack to avoid having
        # everything specified twice
        set(_MAGNUM_${_COMPONENT}INTEGRATION_MAGNUM_DEPENDENCY Shapes)
    endif()

    list(APPEND _MAGNUMINTEGRATION_DEPENDENCIES ${_MAGNUM_${_COMPONENT}INTEGRATION_MAGNUM_DEPENDENCY})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMINTEGRATION_DEPENDENCIES})

find_path(MAGNUMINTEGRATION_INCLUDE_DIR Magnum
    HINTS ${MAGNUM_INCLUDE_DIR})

# Global integration include dir
set(MAGNUMINTEGRATION_INCLUDE_DIRS ${MAGNUMINTEGRATION_INCLUDE_DIR})

# Additional components
foreach(component ${MagnumIntegration_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # Try to find both debug and release version of the library
    find_library(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG Magnum${component}Integration-d)
    find_library(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE Magnum${component}Integration)

    # Set the _LIBRARY variable based on what was found
    if(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG AND MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE)
        set(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY
            debug ${MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG}
            optimized ${MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE})
    elseif(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG)
        set(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY ${MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG})
    elseif(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE)
        set(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY ${MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE})
    endif()

    set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_SUFFIX ${component}Integration)

    # Bullet integration library
    if(${component} STREQUAL Bullet)
        find_package(Bullet)
        if(BULLET_FOUND)
            set(_MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES ${BULLET_LIBRARIES})
            set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES MotionState.h)
            set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS ${BULLET_INCLUDE_DIRS})
        else()
            unset(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY)
        endif()

    # Oculus SDK integration library
    elseif(${component} STREQUAL Ovr)
        find_package(OVR)
        if(OVR_FOUND)
            set(_MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES ${OVR_LIBRARY})
            set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES OvrIntegration.h)
            set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS ${OVR_INCLUDE_DIR})
        else()
            unset(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY)
        endif()
    endif()

    # Try to find the includes
    if(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES)
        find_path(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIR
            NAMES ${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES}
            HINTS ${MAGNUMINTEGRATION_INCLUDE_DIR}/Magnum/${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_SUFFIX})
    endif()

    # Add Magnum library dependency, if there is any
    if(_MAGNUM_${_COMPONENT}INTEGRATION_MAGNUM_DEPENDENCY)
        string(TOUPPER ${_MAGNUM_${_COMPONENT}INTEGRATION_MAGNUM_DEPENDENCY} _DEPENDENCY)
        set(_MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES ${_MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES} ${MAGNUM_${_DEPENDENCY}_LIBRARIES})
        set(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS ${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS} ${MAGNUM_${_DEPENDENCY}_INCLUDE_DIRS})
    endif()

    # Decide if the library was found
    if(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY AND _MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIR)
        set(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES
            ${MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY}
            ${_MAGNUM_${_COMPONENT}INTEGRATION_LIBRARIES})
        set(MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS
            ${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIRS})

        set(MagnumIntegration_${component}_FOUND TRUE)

        # Don't expose variables w/o dependencies to end users
        mark_as_advanced(FORCE
            MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_DEBUG
            MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY_RELEASE
            MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY
            _MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIR)
    else()
        set(MagnumIntegration_${component}_FOUND FALSE)
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumIntegration
    REQUIRED_VARS MAGNUMINTEGRATION_INCLUDE_DIR
    HANDLE_COMPONENTS)

mark_as_advanced(MAGNUMINTEGRATION_INCLUDE_DIR)
