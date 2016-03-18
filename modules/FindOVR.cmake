#.rst:
# Find OVR
# --------
#
# Finds the OVR library. This module defines:
#
#  OVR_FOUND                - True if OVR library is found
#  OVR::OVR                 - OVR imported target
#
# Additionally these variables are defined for internal usage:
#
#  OVR_LIBRARY              - OVR library
#  OVR_INCLUDE_DIR          - Include dir
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2015 Jonathan Hale <squareys@googlemail.com>
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

set(LIBOVR_ROOT ${CMAKE_INSTALL_PREFIX})

if(NOT OVR_SDK_ROOT)
    message(WARNING "OVR_SDK_ROOT is not set. Will try to find headers and library in ${CMAKE_INSTALL_PREFIX}." )
else()
    set(LIBOVR_ROOT ${OVR_SDK_ROOT}/LibOVR)
endif()

# find include directory
find_path(OVR_INCLUDE_DIR NAMES OVR_CAPI.h HINTS ${LIBOVR_ROOT}/Include)

if(WIN32)
    if(MSVC)
        find_library(OVR_LIBRARY NAMES LibOVR HINTS ${LIBOVR_ROOT}/Lib/Windows)
    elseif(MINGW)
        # linking against the MSVC dll with MinGW does not work directly. Instead, you need to
        # link against a specific version. This will cause problems with newer oculus runtimes,
        # though. (FIXME!)
        # The clean way to link against libOVR, which seems to require the Windows DDK
        find_library(OVR_LIBRARY NAMES LibOVRRT32_0_8.dll HINTS "C:/Windows/SysWOW64")
        #find_library(OVR_LIBRARY NAMES LibOVR HINTS ${LIBOVR_ROOT}/Lib/Windows/Win32/Release/VS2012)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR DEFAULT_MSG
    OVR_LIBRARY
    OVR_INCLUDE_DIR)

mark_as_advanced(FORCE
    OVR_LIBRARY
    OVR_INCLUDE_DIR)

if(NOT TARGET OVR::OVR)
    add_library(OVR::OVR UNKNOWN IMPORTED)
    set_target_properties(OVR::OVR PROPERTIES
        IMPORTED_LOCATION ${OVR_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OVR_INCLUDE_DIR})
endif()
