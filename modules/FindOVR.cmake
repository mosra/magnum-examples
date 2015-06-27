# - Find OVR
#
# This module defines:
#
#  OVR_FOUND                - True if OVR library is found
#  OVR_LIBRARY              - OVR dynamic library
#  OVR_INCLUDE_DIR          - Include dir
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2015
#             Jonathan Hale <squareys@googlemail.com>
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
        # The clean way to link against libOVR with mingw would be to compile the linux version,
        # 0.6.0.0-beta is not available for Linux, though.
        find_library(OVR_LIBRARY NAMES LibOVRRT32_0_6.dll HINTS "C:/Windows/SysWOW64")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR OVR_LIBRARY OVR_INCLUDE_DIR)
