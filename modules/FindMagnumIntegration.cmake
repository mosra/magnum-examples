# - Find Magnum integration library
#
# Basic usage:
#  find_package(MagnumIntegration [REQUIRED])
# This command tries to find Magnum integration library and then defines:
#  MAGNUMINTEGRATION_FOUND      - Whether the library was found
# This command alone is useless without specifying the components:
#  Bullet                       - Bullet Physics integration library
# Example usage with specifying additional components is:
#  find_package(MagnumIntegration [REQUIRED|COMPONENTS]
#               Bullet)
# For each component is then defined:
#  MAGNUM_*INTEGRATION_FOUND    - Whether the component was found
#  MAGNUM_*INTEGRATION_LIBRARIES - Component library and dependent libraries
#
# Additionally these variables are defined for internal usage:
#  MAGNUM_*INTEGRATION_LIBRARY  - Component library (w/o dependencies)

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>
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

# Dependencies
find_package(Magnum REQUIRED)

# Additional components
foreach(component ${MagnumIntegration_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # Find the library
    find_library(MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY Magnum${component}Integration)

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
    endif()

    # Try to find the includes
    if(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES)
        find_path(_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIR
            NAMES ${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_NAMES}
            PATHS ${MAGNUM_INCLUDE_DIR}/${_MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_PATH_SUFFIX})
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
            MAGNUM_${_COMPONENT}INTEGRATION_LIBRARY
            _MAGNUM_${_COMPONENT}INTEGRATION_INCLUDE_DIR)
    else()
        set(MagnumIntegration_${component}_FOUND FALSE)
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumIntegration
    REQUIRED_VARS MAGNUM_LIBRARY MAGNUM_INCLUDE_DIR
    HANDLE_COMPONENTS)
