#.rst:
# Find Box2D
# ----------
#
# Finds the Box2D library. This module defines:
#
#  Box2D_FOUND          - True if Box2D library is found
#  Box2D::Box2D         - Box2D imported target
#
# Additionally these variables are defined for internal usage:
#
#  BOX2D_LIBRARY        - Box2D library
#  BOX2D_INCLUDE_DIR    - Include dir
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

# Library
find_library(BOX2D_LIBRARY NAMES Box2D)

# Include dir
find_path(BOX2D_INCLUDE_DIR
    NAMES Box2D/Box2D.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Box2D DEFAULT_MSG
    BOX2D_LIBRARY
    BOX2D_INCLUDE_DIR)

mark_as_advanced(FORCE
    BOX2D_LIBRARY
    BOX2D_INCLUDE_DIR)

if(NOT TARGET Box2D::Box2D)
    add_library(Box2D::Box2D UNKNOWN IMPORTED)
    set_target_properties(Box2D::Box2D PROPERTIES
        IMPORTED_LOCATION ${BOX2D_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${BOX2D_INCLUDE_DIR})
endif()
