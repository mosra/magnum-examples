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

#include <Corrade/Containers/GrowableArray.h>

#include "Objects.h"
#include "Materials.h"
#include "Ray.h"

namespace Magnum { namespace Examples {

bool Sphere::intersect(const Ray& r, Float tMin, Float tMax, HitInfo& hitInfo) const {
    const Vector3 dir = r.unitDirection;
    const Vector3 oc = r.origin - _center;
    const Float a = 1.0f; /* a  = || r ||, and ray diriection r is normalized */
    const Float b = Math::dot(dir, oc);
    const Float c = Math::dot(oc, oc) - _radiusSqr;
    const Float delta = b*b - a*c;

    if(delta > 0.0f) {
        const Float t1 = (-b - Math::sqrt(delta))/a;
        if(t1 < tMax && t1 > tMin) {
            computeHitInfo(r, t1, hitInfo);
            return true;
        }

        const Float t2 = (-b + Math::sqrt(delta))/a;
        if(t2 < tMax && t2 > tMin) {
            computeHitInfo(r, t2, hitInfo);
            return true;
        }
    }
    return false;
}

void Sphere::computeHitInfo(const Ray& r, Float t, HitInfo& hitInfo) const {
    hitInfo.t = t;
    hitInfo.p = r.point(t);
    hitInfo.unitNormal = (hitInfo.p - _center).normalized();
    hitInfo.material = _material.get();
}

bool ObjectList::intersect(const Ray& r, Float tMin, Float tMax, HitInfo& hitInfo) const {
    bool hit = false;
    Float minHitTime = tMax;
    for(std::size_t i = 0; i != _objects.size(); ++i) {
        if(_objects[i]->intersect(r, tMin, minHitTime, hitInfo)) {
            hit = true;
            minHitTime = hitInfo.t;
        }
    }
    return hit;
}

void ObjectList::addObject(Containers::Pointer<Object>&& object) {
    arrayAppend(_objects, Utility::move(object));
}

}}
