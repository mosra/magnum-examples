#ifndef Magnum_Box3DIntegration_Converters_h
#define Magnum_Box3DIntegration_Converters_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2026 Igal Alkon <igal@alkontek.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Quaternion.h>

#include <box3d/box3d.h>

namespace Magnum { namespace Math { namespace Implementation {

/* b3Vec3 / b3Pos are the same type in Box3D (b3Pos is an alias).
   Fields are float; one specialization covers both names. */
template<> struct VectorConverter<3, Float, b3Vec3> {
    static Vector<3, Float> from(const b3Vec3& other) {
        return {other.x, other.y, other.z};
    }
    static b3Vec3 to(const Vector<3, Float>& other) {
        return {other[0], other[1], other[2]};
    }
};

/* b3Quat <-> Quaternion
   Box3D stores quaternion as { b3Vec3 v; float s; } (vector part + scalar) */
template<> struct QuaternionConverter<Float, b3Quat> {
    static Quaternion<Float> from(const b3Quat& other) {
        return {{other.v.x, other.v.y, other.v.z}, other.s};
    }
    static b3Quat to(const Quaternion<Float>& other) {
        return {{other.vector().x(), other.vector().y(), other.vector().z()}, other.scalar()};
    }
};

}}}

#endif
