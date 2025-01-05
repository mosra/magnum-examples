#ifndef Magnum_Examples_FluidSimulation2D_MathHelpers_h
#define Magnum_Examples_FluidSimulation2D_MathHelpers_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix.h>

namespace Magnum { namespace Examples {

template<class T> inline T fractionInside(T phiLeft, T phiRight) {
    if(phiLeft < 0 && phiRight < 0)
        return T(1);

    if(phiLeft < 0 && phiRight >= 0)
        return phiLeft/(phiLeft - phiRight);

    if(phiLeft >= 0 && phiRight < 0)
        return phiRight/(phiRight - phiLeft);
    else
        return T(0);
}

template<class T> inline void barycentric(T x, Int& i, T& f, Int iLow, Int iHigh) {
    T s = Math::floor(x);
    i = static_cast<int>(s);
    if(i < iLow) {
        i = iLow;
        f = 0;
    } else if(i > iHigh - 2) {
        i = iHigh - 2;
        f = 1;
    } else {
        f = T(x - s);
    }
}

template<class T> inline T bilerp(const T& v00, const T& v10, const T& v01,
    const T& v11, T fx, T fy)
{
    return Math::lerp(Math::lerp(v00, v10, fx),
                      Math::lerp(v01, v11, fx),
                      fy);
}

template<class T> inline Math::Vector2<T> bilerpGradient(const T& v00,
    const T& v10, const T& v01, const T& v11, T fx, T fy)
{
    const Math::Vector2<T> f00(fy - T(1), fx - T(1));
    const Math::Vector2<T> f10(T(1) - fy, -fx);
    const Math::Vector2<T> f01(-fy, T(1) - fx);
    const Math::Vector2<T> f11(fy, fx);
    return f00 * v00 + f10 * v10 + f01 * v01 + f11 * v11;
}

template<class T> inline T smoothKernel(T r2, T h2) {
    const T t = T(1) - r2/h2;
    const T tExp3 = t*t*t;
    return Math::max(tExp3, T(0));
}

template<class T> inline T linearKernel(const Math::Vector2<T>& d, T hInv) {
    const T tx = T(1) - Math::abs(d.x() * hInv);
    const T ty = T(1) - Math::abs(d.y() * hInv);
    return Math::max(tx * ty, T(0));
}

}}

#endif
