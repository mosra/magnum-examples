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
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020 Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2015, 2016, 2018 Jonathan Hale <squareys@googlemail.com>
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

if(NOT OVR_SDK_ROOT)
    find_path(OVR_SDK_ROOT OculusSDK)

    if(OVR_SDK_ROOT)
        set(OVR_SDK_ROOT "${OVR_SDK_ROOT}/OculusSDK")
    endif()

endif()

set(LIBOVR_ROOT ${OVR_SDK_ROOT}/LibOVR)

# find include directory
find_path(OVR_INCLUDE_DIR NAMES OVR_CAPI.h HINTS ${LIBOVR_ROOT}/Include)

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # compiling for 64 bit
            set(_OVR_MSVC_ARCH "x64")
        else()
            # compiling for 32 bit
            set(_OVR_MSVC_ARCH "Win32")
        endif()

        # select the correct library folder matching MSVC version
        if (MSVC_VERSION GREATER 1800)
            set(_OVR_MSVC_NAME "VS2015")
        elseif (MSVC_VERSION GREATER 1700)
            set(_OVR_MSVC_NAME "VS2013")
        elseif (MSVC_VERSION GREATER 1600)
            set(_OVR_MSVC_NAME "VS2012")
        else()
            set(_OVR_MSVC_NAME "VS2010")
        endif()

        find_library(OVR_LIBRARY_DEBUG NAMES LibOVR HINTS "${LIBOVR_ROOT}/Lib/Windows/${_OVR_MSVC_ARCH}/Debug/${_OVR_MSVC_NAME}")
        find_library(OVR_LIBRARY_RELEASE NAMES LibOVR HINTS "${LIBOVR_ROOT}/Lib/Windows/${_OVR_MSVC_ARCH}/Release/${_OVR_MSVC_NAME}")

        unset(_OVR_MSVC_ARCH)
        unset(_OVR_MSVC_NAME)
    elseif(MINGW)
        # we cannot link against the MSVC lib with MinGW. Instead, we link directly to the runtime DLL,
        # which requires the Oculus runtime to be installed.
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # compiling for 64 bit
            find_library(OVR_LIBRARY_RELEASE NAMES LibOVRRT64_1.dll HINTS "C:/Program Files (x86)/Oculus/Support/oculus-runtime")
        else()
            # compiling for 32 bit
            find_library(OVR_LIBRARY_RELEASE NAMES LibOVRRT32_1.dll HINTS "C:/Program Files (x86)/Oculus/Support/oculus-runtime")
        endif()
    endif()
else()
    error("The Oculus SDK does not support ${CMAKE_SYSTEM_NAME}.")
endif()

include(SelectLibraryConfigurations)
select_library_configurations(OVR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR DEFAULT_MSG
    OVR_LIBRARY
    OVR_INCLUDE_DIR)

mark_as_advanced(FORCE
    OVR_LIBRARY_DEBUG
    OVR_LIBRARY_RELEASE
    OVR_INCLUDE_DIR)

if(NOT TARGET OVR::OVR)
    add_library(OVR::OVR UNKNOWN IMPORTED)

    set_property(TARGET OVR::OVR APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(OVR::OVR PROPERTIES
        IMPORTED_LOCATION_RELEASE ${OVR_LIBRARY_RELEASE}
        INTERFACE_INCLUDE_DIRECTORIES ${OVR_INCLUDE_DIR})

    if(OVR_LIBRARY_DEBUG)
        set_property(TARGET OVR::OVR APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_property(TARGET OVR::OVR PROPERTY
            IMPORTED_LOCATION_DEBUG ${OVR_LIBRARY_DEBUG})
    endif()
endif()
