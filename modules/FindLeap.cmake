#.rst:
# Find Leap
# --------
#
# Finds the Leap library. This module defines:
#
#  Leap_FOUND                - True if Leap library is found
#  Leap::Leap                 - Leap imported target
#
# Additionally these variables are defined for internal usage:
#
#  Leap_LIBRARY              - Leap library
#  Leap_INCLUDE_DIR          - Include dir
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2018 Jonathan Hale <squareys@googlemail.com>
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

if(NOT LEAP_SDK_ROOT)
    find_path(LEAP_SDK_ROOT LeapSDK)

    if(LEAP_SDK_ROOT)
        set(LEAP_SDK_ROOT "${LEAP_SDK_ROOT}/LeapSDK")
    endif()
endif()

# Find include directory
find_path(Leap_INCLUDE_DIR NAMES Leap.h HINTS ${LEAP_SDK_ROOT}/include)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Compiling for 64 bit
    set(_Leap_ARCH "x64")
else()
    # Compiling for 32 bit
    set(_Leap_ARCH "x86")
endif()

if(MINGW)
    # On Windows with MinGW, we link to "LeapSDK/lib/<arch>/Leap.dll"
    find_library(Leap_LIBRARY NAMES Leap.dll HINTS "${LEAP_SDK_ROOT}/lib/${_Leap_ARCH}")
else()
    # On Linux, "LeapSDK/lib/<arch>/libLeap.so"
    # On Mac, "LeapSDK/lib/libLeap.dylib"
    # On Windows, "LeapSDK/lib/<arch>/Leap.lib"
    find_library(Leap_LIBRARY NAMES Leap libLeap HINTS "${LEAP_SDK_ROOT}/lib/${_Leap_ARCH}" "${LEAP_SDK_ROOT}/lib/")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Leap DEFAULT_MSG
    Leap_LIBRARY
    Leap_INCLUDE_DIR)

mark_as_advanced(FORCE
    Leap_LIBRARY
    Leap_INCLUDE_DIR)

if(NOT TARGET Leap::Leap)
    add_library(Leap::Leap UNKNOWN IMPORTED)
    set_target_properties(Leap::Leap PROPERTIES
        IMPORTED_LOCATION ${Leap_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${Leap_INCLUDE_DIR})
endif()
