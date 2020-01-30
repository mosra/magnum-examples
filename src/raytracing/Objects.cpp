/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
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

#include <Corrade/Containers/GrowableArray.h>

#include "Objects.h"
#include "Materials.h"
#include "Ray.h"

namespace Magnum { namespace Examples {
Sphere::~Sphere() { delete _material; }

bool Sphere::intersect(const Ray& r, Float t_min, Float t_max, HitInfo& hitInfo) const {
    const Vector3 dir   = r.direction;
    const Vector3 oc    = r.origin - _center;
    const Float   a     = Math::dot(dir, dir);
    const Float   b     = 2.0f * dot(dir, oc);
    const Float   c     = Math::dot(oc, oc) - _radius * _radius;
    const Float   delta = b * b - 4.0f * a * c;

    if(delta > 0.0f) {
        Float tmp = (-b - std::sqrt(delta)) / (2.0f * a);
        if(tmp < t_max && tmp > t_min) { /* ray is outside sphere */
            hitInfo.t        = tmp;
            hitInfo.p        = r.point(tmp);
            hitInfo.normal   = (hitInfo.p - _center) / _radius;
            hitInfo.material = _material;
            return true;
        }
        tmp = (-b + std::sqrt(delta)) / (2.0f * a);
        if(tmp < t_max && tmp > t_min) { /* ray is inside sphere */
            hitInfo.t        = tmp;
            hitInfo.p        = r.point(tmp);
            hitInfo.normal   = (hitInfo.p - _center) / _radius;
            hitInfo.material = _material;
            return true;
        }
    }
    return false;
}

bool ObjectList::intersect(const Ray& r, Float t_min, Float t_max, HitInfo& hitInfo) const {
    HitInfo tmpHitInfo;
    bool    bHit       = false;
    Float   minHitTime = t_max;
    for(std::size_t i = 0; i < _objects.size(); ++i) {
        if(_objects[i]->intersect(r, t_min, minHitTime, tmpHitInfo)) {
            bHit       = true;
            minHitTime = tmpHitInfo.t;
            hitInfo    = tmpHitInfo;
        }
    }
    return bHit;
}

void ObjectList::addObject(Object* const object) {
    Containers::arrayAppend(_objects, Containers::Pointer<Object>{ object });
}
} }
