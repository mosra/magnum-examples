#ifndef Magnum_Examples_RayTracing_Camera_h
#define Magnum_Examples_RayTracing_Camera_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include <Magnum/Math/Functions.h>

#include "RndGenerators.h"
#include "Ray.h"

namespace Magnum { namespace Examples {

class Camera {
    public:
        explicit Camera(const Vector3& eye, const Vector3& viewCenter,
            const Vector3& upDir, Deg fov, Float aspectRatio,
            Float lensRadius): _origin{eye}, _lensRadius{lensRadius}
        {
            _w = (eye - viewCenter).normalized();
            _u = Math::cross(upDir, _w).normalized();
            _v = Math::cross(_w, _u).normalized();

            const Float halfHeight = Math::tan(fov*0.5f);
            const Float halfWidth = aspectRatio*halfHeight;
            const Float focusDistance = (eye - viewCenter).length();
            _lowerLeftCorner = _origin - halfWidth*focusDistance*_u -
                halfHeight*focusDistance*_v - focusDistance*_w;
            _horitonalEdge = 2.0f*halfWidth*focusDistance*_u;
            _verticalEdge  = 2.0f*halfHeight*focusDistance*_v;
        }

        Ray ray(Float s, Float t) const {
            const Vector2 rd = _lensRadius*Rnd::rndInDisk();
            const Vector3 offset = _u*rd.x() + _v*rd.y();
            return Ray{_origin + offset, _lowerLeftCorner + s*_horitonalEdge +
                t*_verticalEdge - offset - _origin};
        }

    private:
        Vector3 _origin;
        Float   _lensRadius;
        Vector3 _u, _v, _w;
        Vector3 _lowerLeftCorner;
        Vector3 _horitonalEdge, _verticalEdge;
};
} }

#endif
